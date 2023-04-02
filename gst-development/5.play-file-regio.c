#include <gst/gst.h>

static GMainLoop *loop;
static volatile gint counter;
static GstBus *bus;
static gboolean prerolled = FALSE;
static GstPad *sinkpad;

static void
dec_counter (GstElement * pipeline) {
  if (prerolled)
    return;

  if (g_atomic_int_dec_and_test (&counter)) {
    /* all probes blocked and no-more-pads signaled, post
     * message on the bus. */
    prerolled = TRUE;

    gst_bus_post (bus, gst_message_new_application (
          GST_OBJECT_CAST (pipeline),
          gst_structure_new_empty ("ExPrerolled")));
  }
}

/* called when a source pad of uridecodebin is blocked */
static GstPadProbeReturn
cb_blocked (GstPad          *pad,
            GstPadProbeInfo *info,
            gpointer         user_data)
{
  GstElement *pipeline = GST_ELEMENT (user_data);

  if (prerolled)
    return GST_PAD_PROBE_REMOVE;

  dec_counter (pipeline);

  return GST_PAD_PROBE_OK;
}

/* called when uridecodebin has a new pad */
static void
cb_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    user_data)
{
  GstElement *pipeline = GST_ELEMENT (user_data);

  if (prerolled)
    return;

  g_atomic_int_inc (&counter);

  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      (GstPadProbeCallback) cb_blocked, pipeline, NULL);

  /* try to link to the video pad */
  gst_pad_link (pad, sinkpad);
}

/* called when uridecodebin has created all pads */
static void
cb_no_more_pads (GstElement *element,
                 gpointer    user_data)
{
  GstElement *pipeline = GST_ELEMENT (user_data);

  if (prerolled)
    return;

  dec_counter (pipeline);
}

/* called when a new message is posted on the bus */
static void
cb_message (GstBus *bus, GstMessage *message, gpointer user_data) {
  GstElement *pipeline = GST_ELEMENT (user_data);
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      g_print ("we received an error!\n");
      /* 解析错误信息 */
      gst_message_parse_error (message, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (message->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_EOS:
      g_print ("we reached EOS\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_APPLICATION:
    {
      if (gst_message_has_name (message, "ExPrerolled")) {
        /* it's our message */
        g_print ("we are all prerolled, do seek\n");
        gst_element_seek (pipeline,
            1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
            GST_SEEK_TYPE_SET, 2 * GST_SECOND,
            GST_SEEK_TYPE_SET, 5 * GST_SECOND);

        gst_element_set_state (pipeline, GST_STATE_PLAYING);
      }
      break;
    }
    default:
      break;
  }
}

/**
 * gst-launch-1.0 uridecodebin uri=file:///home/lieryang/Desktop/gst-study/gst-development/sample-480p.webm ! video/x-raw,format=I420 ! videoconvert ! autovideosink
*/ 

gint
main (gint argc, gchar *argv[]) {

  GstElement *pipeline, *src, *csp, *vs, *sink;

  /* init GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* build */
  pipeline = gst_pipeline_new ("my-pipeline");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", (GCallback) cb_message, pipeline);

  src = gst_element_factory_make ("uridecodebin", "src");
  if (src == NULL)
    g_error ("Could not create 'uridecodebin' element");

  g_object_set (src, "uri", "file:///home/lieryang/Desktop/gst-study/gst-development/sample_720p.mp4", NULL);

  csp = gst_element_factory_make ("videoconvert", "csp");
  if (csp == NULL)
    g_error ("Could not create 'videoconvert' element");

  vs = gst_element_factory_make ("videoscale", "vs");
  if (csp == NULL)
    g_error ("Could not create 'videoscale' element");

  sink = gst_element_factory_make ("autovideosink", "sink");
  if (sink == NULL)
    g_error ("Could not create 'autovideosink' element");

  gst_bin_add_many (GST_BIN (pipeline), src, csp, vs, sink, NULL);

  /* can't link src yet, it has no pads */
  gst_element_link_many (csp, vs, sink, NULL);

  sinkpad = gst_element_get_static_pad (csp, "sink");

  /* for each pad block that is installed, we will increment
   * the counter. for each pad block that is signaled, we
   * decrement the counter. When the counter is 0 we post
   * an app message to tell the app that all pads are
   * blocked. Start with 1 that is decremented when no-more-pads
   * is signaled to make sure that we only post the message
   * after no-more-pads */
  /**
   * 对于安装的每个pad阻塞，我们都将计数器加1。
   * 对于每个发出信号的pad阻塞，我们将计数器减1。
   * 当计数器为0时，我们发布一条消息，告诉应用程序所有的pad都被阻塞了。
   * 从1开始，当发出no-more-pads信号时将其减1
   * 以确保我们只在no-more-pads之后发布消息
  */
  g_atomic_int_set (&counter, 1);

  g_signal_connect (src, "pad-added", (GCallback) cb_pad_added, pipeline);
  g_signal_connect (src, "no-more-pads", (GCallback) cb_no_more_pads, pipeline);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (sinkpad);
  gst_object_unref (bus);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);

  return 0;
}
