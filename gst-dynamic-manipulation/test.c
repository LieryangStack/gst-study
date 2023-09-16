#include <gst/gst.h>

//GST_DEBUG_CATEGORY_STATIC (my_category);
//#define GST_CAT_DEFAULT my_category

typedef struct {
  GMainLoop *loop;
  GstElement *pipeline;

  /*解码*/
  GstElement *rtsp_src;
  GstElement *rtph264_depay;
  GstElement *h264_parse;
  GstElement *nvv4l2_decoder;
  GstElement *nvvideo_convert;
  GstElement *cap_filter;
  GstElement *queue;
  GstElement *tee;
  GstPad *tee_src0_pad;
  GstPad *tee_src1_pad;

  /* 播放视频 */
  GstElement *video_queue;
  GstElement *nvegl_transform;
  GstElement *nveglgles_sink;

  /* 保存文件 */
  GstElement *file_queue;
  GstElement *nvv4l2_h264enc;
  GstElement *file_h264_parse;
  GstElement *qtmux;
  GstElement *file_sink;

} CustomData;

static guint counter;

static gboolean
create_encode_file_bin (CustomData * data) {
  gboolean ret = FALSE;
  gchar file_name[50];

  data->file_queue = gst_element_factory_make("queue", "file_queue");
  data->nvv4l2_h264enc = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
  data->qtmux = gst_element_factory_make("qtmux", "qtmux");
  data->file_sink = gst_element_factory_make("filesink", "file_sink");

  if (!data->file_queue || !data->nvv4l2_h264enc || \
      !data->qtmux || !data->file_sink ) {
    g_printerr ("Not all elements could be created.\n");
    goto done;
  }

  gst_bin_add_many (GST_BIN (data->pipeline), data->file_queue, data->nvv4l2_h264enc, 
                                              data->qtmux, data->file_sink,  NULL);

  g_snprintf (file_name, sizeof (file_name), "test%d.mp4", counter++);
  g_object_set (G_OBJECT (data->file_sink), "location", file_name, NULL);
  g_object_set (G_OBJECT (data->file_sink), "sync", FALSE, "max-lateness", -1, "async", FALSE, NULL);

  if (gst_element_link_many (data->file_queue, data->nvv4l2_h264enc, \
                             data->qtmux, data->file_sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    goto done;
  }

  ret = TRUE;

done:
  if (!ret) {
    g_printerr ("%s failed", __func__);
  }
  return ret;
}

static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  CustomData *data = (CustomData *)user_data;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  //gst_element_set_state (data->sink_sub_bin, GST_STATE_NULL);
  GstBus *bus = gst_element_get_bus (data->pipeline);
  gst_bus_post (bus, gst_message_new_application (GST_OBJECT_CAST (data->pipeline),
                                            gst_structure_new_empty ("PAUSED")));
  gst_object_unref (bus);
  
  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *tee_sink_pad, *file_sink_pad;
  CustomData *data = (CustomData *)user_data;

  GST_DEBUG_OBJECT (pad, "pad is blocked now");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* install new probe for EOS */
  file_sink_pad = gst_element_get_static_pad (data->file_queue, "sink");
  gst_pad_add_probe (file_sink_pad, GST_PAD_PROBE_TYPE_BLOCK |
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, user_data, NULL);
  gst_object_unref (file_sink_pad);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  tee_sink_pad = gst_element_get_static_pad (data->file_queue, "sink");
  gst_pad_send_event (tee_sink_pad, gst_event_new_eos ());
  gst_object_unref (tee_sink_pad);

  return GST_PAD_PROBE_OK;
}

static gboolean
timeout_cb (gpointer user_data) {                  
  CustomData *data = (CustomData *)user_data;
  GstPad *src_pad = gst_element_get_static_pad (data->queue, "src");
  /*阻塞rtspsrc元素*/
  gst_pad_add_probe (src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      pad_probe_cb, user_data, NULL);
  gst_object_unref (src_pad);
  
  return TRUE;
}

static gboolean
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data) {
  CustomData *data = (CustomData *)user_data;
  GstPad *bin_sink_pad;
  GError *err;
  gchar *debug_info;
  gint ret;

  switch (GST_MESSAGE_TYPE (msg)) {
    GstState old_state, new_state, pending_state;
    case GST_MESSAGE_STATE_CHANGED:
     g_print (">>>>>> Message: name: %s, source name: %s\n", GST_MESSAGE_TYPE_NAME(msg),
                                                      GST_MESSAGE_SRC_NAME(msg));
      /* We are only interested in state-changed messages from the pipeline */
      //if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) 
      gst_message_parse_state_changed (msg, &old_state, &new_state,
        &pending_state);
      g_print ("Pipeline state changed from %s to %s:\n",
        gst_element_state_get_name (old_state),
        gst_element_state_get_name (new_state));
      break;
    case GST_MESSAGE_ERROR:
      /* 解析错误信息 */
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n",
          debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      g_main_loop_quit (data->loop);
      break;
    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream reached.\n");
      break;
    case GST_MESSAGE_QOS:
      g_print (">>>>>> Message: name: %s, source name: %s\n", GST_MESSAGE_TYPE_NAME(msg),
                                                      GST_MESSAGE_SRC_NAME(msg));
      break;
    case GST_MESSAGE_CLOCK_LOST:
      /* Get a new clock */
      g_print (">>>>>> Message: name: %s, source name: %s\n", GST_MESSAGE_TYPE_NAME(msg),
                                                      GST_MESSAGE_SRC_NAME(msg));
      gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
      gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
      break;
    case GST_MESSAGE_APPLICATION :
      if (gst_message_has_name (msg, "PAUSED")) {

        gst_element_set_state (data->file_queue, GST_STATE_NULL);
        gst_element_set_state (data->nvv4l2_h264enc, GST_STATE_NULL);
        gst_element_set_state (data->file_h264_parse, GST_STATE_NULL);
        gst_element_set_state (data->qtmux, GST_STATE_NULL);
        gst_element_set_state (data->file_sink, GST_STATE_NULL);
        g_print ("change = %d\n", ret);
        
        /*移除就会自动断开链接和解引用*/
        gst_bin_remove (GST_BIN (data->pipeline), data->file_queue);
        gst_bin_remove (GST_BIN (data->pipeline), data->nvv4l2_h264enc);
        gst_bin_remove (GST_BIN (data->pipeline), data->file_h264_parse);
        gst_bin_remove (GST_BIN (data->pipeline), data->qtmux);
        gst_bin_remove (GST_BIN (data->pipeline), data->file_sink);

        gst_element_release_request_pad (data->tee, data->tee_src1_pad);
        gst_object_unref (data->tee_src1_pad);

        /* 录像 */
        create_encode_file_bin (data);

        gst_element_sync_state_with_parent (data->file_queue);
        gst_element_sync_state_with_parent (data->nvv4l2_h264enc);
        gst_element_sync_state_with_parent (data->file_h264_parse);
        gst_element_sync_state_with_parent (data->qtmux);
        gst_element_sync_state_with_parent (data->file_sink);

        data->tee_src1_pad = gst_element_get_request_pad (data->tee, "src_1");
        bin_sink_pad = gst_element_get_static_pad (data->file_queue, "sink");
        gst_pad_link (data->tee_src1_pad, bin_sink_pad);

        //gst_element_set_state (data->pipeline, GST_STATE_READY);
        //ret = gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
        g_print ("******************************change**************************** = %d\n", ret);
    }
      break;
    default:
      /* We should not reach here because we only asked for ERRORs and EOS */
      g_printerr ("Unexpected message id = %s received.\n", GST_MESSAGE_TYPE_NAME(msg));
      break;
  }
  return TRUE;
}


int main (int argc, char *argv[]) {
  
  GstPad *video_queue_pad, *bin_sink_pad;

  GstCaps *caps = NULL;

#if 1
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", "./", TRUE);

  g_setenv ("GST_DEBUG_NO_COLOR", "1", TRUE);
  g_setenv ("GST_DEBUG_FILE", "pipeline.log", TRUE);
  g_setenv ("GST_DEBUG", "2", TRUE);
#endif

  CustomData *data = g_malloc0 (sizeof(CustomData));
  gst_init (&argc, &argv);
  
  /* gst初始化后初始 , 0日志字符表示无颜色输出， 1表示有颜色输出*/
  //GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

  data->rtsp_src = gst_element_factory_make("videotestsrc", "videotestsrc");
  data->nvvideo_convert = gst_element_factory_make("nvvideoconvert", "nvvideo_convert");
  data->queue = gst_element_factory_make ("queue", "queue");
  data->tee = gst_element_factory_make("tee", "tee");

  data->video_queue = gst_element_factory_make("queue", "video_queue");
  data->nveglgles_sink = gst_element_factory_make("nveglglessink", "nveglglessink");

  /* Create the empty pipeline */
  data->pipeline = gst_pipeline_new ("test-pipeline");

  if (!data->pipeline || !data->rtsp_src || !data->nvvideo_convert || !data->queue || \
      !data->tee || !data->video_queue || !data->nveglgles_sink) {
    g_printerr ("Not all elements could be created.\n");
    goto exit;
  }

  /* Link all elements that can be automatically linked because they have "Always" pads */
  gst_bin_add_many (GST_BIN (data->pipeline), data->rtsp_src, data->nvvideo_convert,\
                    data->queue, data->tee, data->video_queue, data->nveglgles_sink, NULL);

  /**
   * rtsp_src 与 rtph264_depay 通过回调函数连接
   * tee 与 file_queue 和 video_queue 通过请求Pad链接
  */
  if (gst_element_link_many (data->rtsp_src, data->nvvideo_convert, data->queue, data->tee, NULL) != TRUE ||
      gst_element_link_many (data->video_queue, data->nveglgles_sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    goto exit;
  }

  /* Manually link the Tee, which has "Request" pads */
  data->tee_src0_pad = gst_element_get_request_pad (data->tee, "src_0");
  video_queue_pad = gst_element_get_static_pad (data->video_queue, "sink");

  if (gst_pad_link (data->tee_src0_pad, video_queue_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Tee could not be linked.\n");
    goto exit;
  }
  gst_object_unref (video_queue_pad);

    /* 增加录像 */
  create_encode_file_bin (data);

  data->tee_src1_pad = gst_element_get_request_pad (data->tee, "src_1");
  bin_sink_pad = gst_element_get_static_pad (data->file_queue, "sink");
  gst_pad_link (data->tee_src1_pad, bin_sink_pad);

  g_object_set (G_OBJECT (data->rtsp_src), "is-live", FALSE, NULL);

  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);


  data->loop = g_main_loop_new (NULL, FALSE);
  gst_bus_add_watch (GST_ELEMENT_BUS (data->pipeline), bus_cb, data);
  g_timeout_add_seconds (3, timeout_cb, data);

  g_main_loop_run (data->loop);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

  GST_WARNING_OBJECT (data->pipeline, "GST_STATE_NULL");
  gst_element_set_state (data->pipeline, GST_STATE_NULL);

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad (data->tee, data->tee_src0_pad);
  gst_element_release_request_pad (data->tee, data->tee_src1_pad);
  gst_object_unref (data->tee_src0_pad);
  gst_object_unref (data->tee_src1_pad);

  GST_WARNING_OBJECT (data->pipeline, "Exiting...");

exit:
  g_print ("Exiting...");

  if (caps) {
    gst_caps_unref (caps);
    caps = NULL;
  }
  if (data->pipeline)
    gst_object_unref (data->pipeline);
  
  return 0;
}
