/**
 * @brief: 改进文件6的方案，实现动态控制bin
*/

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>
#include "deepstream_common.h"

GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category
#define DEBUG_LOG_DOT_FLAG TRUE 

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
  gulong queue_probe_id;

  /* 播放视频 */
  GstElement *video_queue;
  GstElement *nvegl_transform;
  GstElement *nveglgles_sink;

  /* 保存文件 */
  GstElement *sink_sub_bin;
  GstElement *file_queue;
  GstElement *nvv4l2_h264enc;
  GstElement *file_h264_parse;
  GstElement *qtmux;
  GstElement *file_sink;

} CustomData;

static guint counter;

static void
destory_encode_file_bin (CustomData *data) {
  /* step1.移除 */
  /*移除就会自动断开链接和解引用*/
  gst_bin_remove (GST_BIN (data->sink_sub_bin), data->file_queue);
  gst_bin_remove (GST_BIN (data->sink_sub_bin), data->nvv4l2_h264enc);
  gst_bin_remove (GST_BIN (data->sink_sub_bin), data->file_h264_parse);
  gst_bin_remove (GST_BIN (data->sink_sub_bin), data->qtmux);
  gst_bin_remove (GST_BIN (data->sink_sub_bin), data->file_sink);

  /* step2.设定NULL状态 */
  gst_element_set_state (data->file_queue, GST_STATE_NULL);
  gst_element_set_state (data->nvv4l2_h264enc, GST_STATE_NULL);
  gst_element_set_state (data->file_h264_parse, GST_STATE_NULL);
  gst_element_set_state (data->qtmux, GST_STATE_NULL);
  gst_element_set_state (data->file_sink, GST_STATE_NULL);
  
  g_print ("**************GST_OBJECT_REFCOUNT_VALUE(data->file_sink) = %d***********\n", GST_OBJECT_REFCOUNT_VALUE(data->file_sink));

  /* step3.解引用&指针赋值NULL */
  gst_clear_object (&(data->file_queue));
  gst_clear_object (&(data->nvv4l2_h264enc));
  gst_clear_object (&(data->file_h264_parse));
  gst_clear_object (&(data->qtmux));
  gst_clear_object (&(data->file_sink));
}


static gboolean
create_encode_file_bin (CustomData * data) {
  gboolean ret = FALSE;
  gchar file_name[50];

  data->sink_sub_bin = gst_bin_new ("sink_sub_bin");
  if (!data->sink_sub_bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->sink_sub_bin));
    goto done;
  }

  data->file_queue = gst_element_factory_make("queue", "file_queue");
  if (!data->file_queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->file_queue));
    goto done;
  }

  data->nvv4l2_h264enc = gst_element_factory_make("nvv4l2h264enc", "nvv4l2_h264enc");
  if (!data->nvv4l2_h264enc) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->nvv4l2_h264enc));
    goto done;
  }

  data->file_h264_parse = gst_element_factory_make("h264parse", "file_h264_parse");
  if (!data->file_h264_parse) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->file_h264_parse));
    goto done;
  }

  data->qtmux = gst_element_factory_make("qtmux", "qtmux");
  if (!data->qtmux) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->qtmux));
    goto done;
  }

  data->file_sink = gst_element_factory_make("filesink", "file_sink");
  if (!data->file_sink) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", GST_ELEMENT_NAME(data->file_sink));
    goto done;
  }
  /* GstElement元素是浮点引用， gst_bin_add_many之后，因为是浮点引用，不会增加ref +1
   * 直接对浮点引用 gst_object_ref ，他还是浮点引用，引用数会加+1
   */
  g_message ("g_object_is_floating (data->file_sink) : %d\n", g_object_is_floating (data->file_sink));
  g_message ("Create after: GST_OBJECT_REFCOUNT_VALUE (data->file_sink) = %d\n",GST_OBJECT_REFCOUNT_VALUE (data->file_sink));

  /* 因为释放的时候，我采取先移除，再设定NULL，最后clear解引用 */
  /* gst_bin_add_many会把浮动引用变为正常引用，再次之后ref = 2*/
  gst_bin_add_many (GST_BIN (gst_object_ref(data->sink_sub_bin)), gst_object_ref (data->file_queue), \
                    gst_object_ref (data->nvv4l2_h264enc), gst_object_ref (data->file_h264_parse), \
                    gst_object_ref (data->qtmux), gst_object_ref (data->file_sink), NULL); 
  g_message ("g_object_is_floating (data->file_sink) : %d\n", g_object_is_floating (data->file_sink));
  g_message ("gst_bin_add_many after: GST_OBJECT_REFCOUNT_VALUE (data->file_sink) = %d\n",GST_OBJECT_REFCOUNT_VALUE (data->file_sink));

  g_snprintf (file_name, sizeof (file_name), "test-%d.mp4", counter++);
  g_object_set (G_OBJECT (data->file_sink), "location", file_name, NULL);
  g_object_set (G_OBJECT (data->file_sink), "async", FALSE, NULL);

  // gst_base_parse_set_infer_ts (data->file_h264_parse, TRUE);
  // gst_base_parse_set_pts_interpolation (data->file_h264_parse, TRUE);

  if (gst_element_link_many (data->file_queue, data->nvv4l2_h264enc, data->file_h264_parse,
                             data->qtmux, data->file_sink, NULL) != TRUE) {
    NVGSTDS_ERR_MSG_V ("Elements on sink_sub_bin could not be linked.\n");
    goto done;
  }

  NVGSTDS_BIN_ADD_GHOST_PAD (data->sink_sub_bin, data->file_queue, "sink");

  ret = TRUE;

done:
  if (!ret) {
    g_printerr ("%s failed", __func__);
  }
  return ret;
}


/**
 * @brief: 通过pad增加信号，连接rtspsrc和rtph264depay
*/
static void
pad_added_handler (GstElement *src, GstPad *new_pad, gpointer user_data) { 
  GstPad *sink_pad;
  CustomData *data = (CustomData *)user_data;
  sink_pad = gst_element_get_static_pad (data->rtph264_depay, "sink");

  if (gst_pad_link (new_pad, sink_pad) != GST_PAD_LINK_OK )
    NVGSTDS_ERR_MSG_V ("%s and %s could not be linked.\n",GST_ELEMENT_NAME (data->rtsp_src), \
                                                    GST_ELEMENT_NAME (data->rtph264_depay));

  gst_object_unref (sink_pad);
}


/**
 * @brief: 检测eos事件
*/
static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data) {
  CustomData *data = (CustomData *)user_data;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  //gst_element_set_state (data->sink_sub_bin, GST_STATE_NULL);
  GstBus *bus = gst_element_get_bus (data->pipeline);
  gst_bus_post (bus, gst_message_new_application (GST_OBJECT_CAST (data->pipeline),
                                            gst_structure_new_empty ("PAUSED")));
  gst_object_unref (bus);
  
  return GST_PAD_PROBE_OK;
}


/**
 * @brief: queue的阻塞回调函数：
 *         仅仅是阻塞数据
*/
static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *sink_pad, *file_sink_pad;
  CustomData *data = (CustomData *)user_data;

  return GST_PAD_PROBE_OK;
}


/**
 * @brief: tee_src_1_pad 的空闲回调函数
 *         1.断开Pad链接
 *         2.发送EOS消息
 *         3.file sink添加event监听函数
*/
static GstPadProbeReturn
pad_idle_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data) {
  CustomData *data = (CustomData *)user_data;
	g_print("Unlinking...");

	GstPad *sinkpad;
	sinkpad = gst_element_get_static_pad (data->sink_sub_bin, "sink");
	gst_pad_unlink (data->tee_src1_pad, sinkpad);
  gst_pad_send_event (sinkpad, gst_event_new_eos ());
	gst_object_unref (sinkpad);

  sinkpad = gst_element_get_static_pad (data->file_sink, "sink");
  gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, user_data, NULL);
  gst_object_unref (sinkpad);

	return GST_PAD_PROBE_REMOVE;
}


/**
 * @brief: 主线程超时回调函数：
 *         1.注册queue阻塞监听函数
 *         2.注册tee空闲监听函数
*/
static gboolean
timeout_cb (gpointer user_data) {                  
  CustomData *data = (CustomData *)user_data;
  /* 1.注册queue阻塞监听函数 */
  GstPad *src_pad = gst_element_get_static_pad (data->queue, "src");
  data->queue_probe_id = gst_pad_add_probe (src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                                               pad_probe_cb, user_data, NULL);
  /* 2.注册tee空闲监听函数 */
  gst_pad_add_probe (data->tee_src1_pad, GST_PAD_PROBE_TYPE_IDLE,
                         pad_idle_probe_cb, user_data, NULL);                                          
  gst_object_unref (src_pad);
  
  return TRUE;
}


static gboolean
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data) {
  CustomData *data = (CustomData *)user_data;
  GstPad *src_pad;
  GstPad *sink_pad;
  GError *err;
  gchar *debug_info;
  gint ret;
  const GstStructure *s;

  switch (GST_MESSAGE_TYPE (msg)) {
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
      gst_element_send_event (data->pipeline, gst_event_new_eos());
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (data->loop);
      g_print ("End-Of-Stream reached.\n");
      break;
    case GST_MESSAGE_QOS:
      g_print (">>>>>> Message: name: %s, source name: %s\n", GST_MESSAGE_TYPE_NAME(msg),
                                                      GST_MESSAGE_SRC_NAME(msg));
      break;
    case GST_MESSAGE_ELEMENT: 
    	s = gst_message_get_structure (msg);
    	if (gst_structure_has_name (s, "GstBinForwarded")) {
	        GstMessage *forward_msg = NULL;

	        gst_structure_get (s, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
	        if (GST_MESSAGE_TYPE (forward_msg) == GST_MESSAGE_EOS) {
	            g_print ("EOS from element %s\n", GST_OBJECT_NAME (GST_MESSAGE_SRC (forward_msg)));	
	        }
	        gst_message_unref (forward_msg);
    	}
    	break;
    case GST_MESSAGE_APPLICATION :
      if (gst_message_has_name (msg, "PAUSED")) {

        /*移除就会自动断开链接和解引用*/
        gst_bin_remove (GST_BIN (data->pipeline), data->sink_sub_bin);
        destory_encode_file_bin (data);
        gst_element_set_state (data->sink_sub_bin, GST_STATE_NULL);
        g_print ("**************GST_OBJECT_REFCOUNT_VALUE(data->sink_sub_bin) = %d***********\n", GST_OBJECT_REFCOUNT_VALUE(data->sink_sub_bin));
        gst_clear_object (&(data->sink_sub_bin));
        
        /*释放tee pad*/
        gst_element_release_request_pad (data->tee, data->tee_src1_pad);
        gst_clear_object (&(data->tee_src1_pad));

        /* 录像 */
        create_encode_file_bin (data);
        gst_bin_add (GST_BIN (data->pipeline), data->sink_sub_bin);
        gst_element_sync_state_with_parent (data->sink_sub_bin);


        data->tee_src1_pad = gst_element_request_pad_simple (data->tee, "src_1");
        sink_pad = gst_element_get_static_pad (data->sink_sub_bin, "sink");
        gst_pad_link (data->tee_src1_pad, sink_pad);

        /* 移除堵塞 */
        src_pad = gst_element_get_static_pad (data->queue, "src");   
        gst_pad_remove_probe (src_pad, data->queue_probe_id);                                     
        gst_object_unref (src_pad);
      }
      break;
    default:
      /* We should not reach here because we only asked for ERRORs and EOS */
      //g_printerr ("Unexpected message id = %s received.\n", GST_MESSAGE_TYPE_NAME(msg));
      break;
  }
  return TRUE;
}


int main (int argc, char *argv[]) {
  GstPad *video_queue_pad, *bin_sink_pad;
  GstCaps *caps = NULL;

#if DEBUG_LOG_DOT_FLAG
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", "./", TRUE);

  g_setenv ("GST_DEBUG_NO_COLOR", "1", TRUE);
  g_setenv ("GST_DEBUG_FILE", "pipeline.log", TRUE);
  g_setenv ("GST_DEBUG", "2", TRUE);
#endif

  CustomData *data = g_new0 (CustomData, 1);
  gst_init (&argc, &argv);
  
  /* gst初始化后初始 , 0日志字符表示无颜色输出， 1表示有颜色输出*/
  GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

  data->rtsp_src = gst_element_factory_make("rtspsrc", "rtsp_src");
  data->rtph264_depay = gst_element_factory_make("rtph264depay", "rtph264_depay");
  data->h264_parse = gst_element_factory_make("h264parse", "h264_parse");
  data->nvv4l2_decoder = gst_element_factory_make("nvv4l2decoder", "nvv4l2_decoder");
  data->nvvideo_convert = gst_element_factory_make("nvvideoconvert", "nvvideo_convert");
  data->cap_filter = gst_element_factory_make ("capsfilter", "cap_filter");
  data->queue = gst_element_factory_make ("queue", "queue");
  data->tee = gst_element_factory_make("tee", "tee");

  data->video_queue = gst_element_factory_make("queue", "video_queue");
  data->nvegl_transform = gst_element_factory_make("nvegltransform", "nvegl_transform");
  data->nveglgles_sink = gst_element_factory_make("nveglglessink", "nveglgles_sink");

  /* Create the empty pipeline */
  data->pipeline = gst_pipeline_new ("test-pipeline");

  if (!data->pipeline || !data->rtsp_src || !data->rtph264_depay || !data->h264_parse || !data->nvv4l2_decoder || !data->nvvideo_convert || \
      !data->cap_filter || !data->queue ||!data->tee || !data->video_queue || !data->nvegl_transform || !data->nveglgles_sink) {
    NVGSTDS_ERR_MSG_V ("Not all elements could be created.\n");
    goto exit;
  }
  
  caps = gst_caps_from_string ("video/x-raw(memory:NVMM), format=I420, with=600, height=500");
  g_object_set (G_OBJECT (data->cap_filter), "caps", caps, NULL);
  g_object_set (G_OBJECT (data->rtsp_src), "location", "rtsp://admin:yangquan123@192.168.10.11:554", NULL);
  g_object_set (G_OBJECT (data->nveglgles_sink), "sync", FALSE, NULL);
  
  g_signal_connect (data->rtsp_src, "pad-added", G_CALLBACK (pad_added_handler), data);
 
  gst_bin_add_many (GST_BIN (data->pipeline), data->rtsp_src, data->rtph264_depay, data->h264_parse, data->nvv4l2_decoder, data->nvvideo_convert, \
                                   data->cap_filter, data->queue, data->tee, data->video_queue, data->nvegl_transform, data->nveglgles_sink, NULL);

  // gst_base_parse_set_infer_ts (data->h264_parse, TRUE);
  // gst_base_parse_set_pts_interpolation (data->h264_parse, TRUE);

  /**
   * rtsp_src 与 rtph264_depay 通过回调函数连接
   * tee 与 file_queue 和 video_queue 通过请求Pad链接
  */
  if (gst_element_link_many (data->rtph264_depay, data->h264_parse, data->nvv4l2_decoder, \
                             data->nvvideo_convert, data->cap_filter, data->queue, data->tee, NULL) != TRUE ||
      gst_element_link_many (data->video_queue, data->nvegl_transform, data->nveglgles_sink, NULL) != TRUE) {
    NVGSTDS_ERR_MSG_V ("Elements could not be linked.\n");
    goto exit;
  }

  /* 手动连接tee元素请求pad */
  data->tee_src0_pad = gst_element_request_pad_simple (data->tee, "src_0");
  video_queue_pad = gst_element_get_static_pad (data->video_queue, "sink");
  if (gst_pad_link (data->tee_src0_pad, video_queue_pad) != GST_PAD_LINK_OK) {
    NVGSTDS_ERR_MSG_V ("Tee could not be linked.\n");
    goto exit;
  }
  gst_clear_object (&video_queue_pad);
  
  /* 增加录像 */
  create_encode_file_bin (data);
  gst_bin_add (GST_BIN (data->pipeline), data->sink_sub_bin);
  gst_element_sync_state_with_parent (data->sink_sub_bin);

  /* 录像bin连接tee src pad */
  data->tee_src1_pad = gst_element_request_pad_simple (data->tee, "src_1");
  bin_sink_pad = gst_element_get_static_pad (data->sink_sub_bin, "sink");
  gst_pad_link (data->tee_src1_pad, bin_sink_pad);

  /* 设定管道播放状态 */
  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);


  data->loop = g_main_loop_new (NULL, FALSE);
  gst_bus_add_watch (GST_ELEMENT_BUS (data->pipeline), bus_cb, data);
  g_timeout_add_seconds (3, timeout_cb, data);

  g_main_loop_run (data->loop);

  //GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

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
