#include <gst/gst.h>

static gchar *opt_effects = NULL;

#define DEFAULT_EFFECTS "identity,exclusion,navigationtest," \
    "agingtv,videoflip,vertigotv,gaussianblur,shagadelictv,edgetv"

static GstPad *queue_1_src_pad;
static GstElement *conv_before;
static GstElement *conv_after;
static GstElement *effect_a;
static GstElement *effect_b;
static GstElement *pipeline;
static gboolean effect_flag;

static GQueue effects = G_QUEUE_INIT;

static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GMainLoop *loop = user_data;
  GstElement *cur_effect;
  GstElement *next;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  if (effect_flag == 0) {
    effect_b = gst_element_factory_make ("gaussianblur", "effect_b");
    gst_element_set_state (effect_a, GST_STATE_NULL);
    gst_bin_remove (GST_BIN (pipeline), effect_a);
    gst_bin_add (GST_BIN (pipeline), effect_b);
    gst_element_link_many (conv_before, effect_b, conv_after, NULL);
    gst_element_set_state (effect_b, GST_STATE_PLAYING);
    effect_flag = 1;
    g_print ("effect_a ->  effect_b\n");
  }
  else {
    effect_a = gst_element_factory_make ("shagadelictv", "effect_a");
    gst_element_set_state (effect_b, GST_STATE_NULL);
    gst_bin_remove (GST_BIN (pipeline), effect_b);
    gst_bin_add (GST_BIN (pipeline), effect_a);
    gst_element_link_many (conv_before, effect_a, conv_after, NULL);
    /* 不应该是先设定状态再连接吗？ */
    gst_element_set_state (effect_a, GST_STATE_PLAYING);
    g_print ("effect_b ->  effect_a\n");
    effect_flag = 0;
  }

  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (pad, "pad is blocked now");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /*给要更换元素的sink发送eos信息，监听被更换元素的srcpad*/
  if (effect_flag == 0 ) { /* 目前是effect_a */
    srcpad = gst_element_get_static_pad (effect_a, "src");
    sinkpad = gst_element_get_static_pad (effect_a, "sink");
  }
  else {
    srcpad = gst_element_get_static_pad (effect_b, "src");
    sinkpad = gst_element_get_static_pad (effect_b, "sink");
  }

  /* install new probe for EOS */
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK |
    GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, user_data, NULL);
  gst_object_unref (srcpad);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  gst_pad_send_event (sinkpad, gst_event_new_eos ());
  gst_object_unref (sinkpad);

  return GST_PAD_PROBE_OK;
}

static gboolean
timeout_cb (gpointer user_data)
{
  gst_pad_add_probe (queue_1_src_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      pad_probe_cb, user_data, NULL);

  return TRUE;
}

static gboolean
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = user_data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *dbg;
      GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "6.dynamic-change-element");
      gst_message_parse_error (msg, &err, &dbg);
      gst_object_default_error (msg->src, err, dbg);
      g_clear_error (&err);
      g_free (dbg);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

int
main (int argc, char **argv) {
  GError *err = NULL;
  GMainLoop *loop;
  GstElement *videotestsrc, *capsfilter, *queue_1, *queue_2, *sink;

  g_setenv("GST_DEBUG_DUMP_DOT_DIR", "./", TRUE);
  gst_init (&argc, &argv);

  pipeline = gst_pipeline_new ("pipeline");

  videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  /* 阻塞的是这个queue */
  queue_1 = gst_element_factory_make ("queue", "queue_1");
  conv_before = gst_element_factory_make ("videoconvert", "conv_before");
  /* 更换的是这个元素 */
  effect_a = gst_element_factory_make ("shagadelictv", "effect_a");
  conv_after = gst_element_factory_make ("videoconvert", "conv_after");
  queue_2 = gst_element_factory_make ("queue", NULL);
  sink = gst_element_factory_make ("ximagesink", NULL);

  
  gst_util_set_object_arg (G_OBJECT (capsfilter), "caps",
    "video/x-raw, width=320, height=240, "
    "format={ I420, YV12, YUY2, UYVY, AYUV, Y41B, Y42B, "
    "YVYU, Y444, v210, v216, NV12, NV21, UYVP, A420, YUV9, YVU9, IYU1 }");
  g_object_set (videotestsrc, "is-live", TRUE, NULL);
  queue_1_src_pad = gst_element_get_static_pad (queue_1, "src");

  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, capsfilter, queue_1, conv_before, effect_a,
      conv_after, queue_2, sink, NULL);

  gst_element_link_many (videotestsrc, capsfilter, queue_1, conv_before, effect_a,
      conv_after, queue_2, sink, NULL);

  effect_flag = 0;

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  loop = g_main_loop_new (NULL, FALSE);

  gst_bus_add_watch (GST_ELEMENT_BUS (pipeline), bus_cb, loop);

  g_timeout_add_seconds (1, timeout_cb, loop);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}