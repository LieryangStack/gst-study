/**
 * 源码地址：https://github.com/sdroege/gst-snippets/blob/217ae015aaddfe3f7aa66ffc936ce93401fca04e/dynamic-filter.c
 * 通过 GST_PAD_PROBE_TYPE_IDLE （没有在被更换filter元素上发送EOS消息）
*/
#include <string.h>
#include <gst/gst.h>

static GMainLoop *loop;
static GstElement *pipeline;
static GstElement *src, *dbin, *conv, *scale, *navseek, *queue, *sink;
static GstElement *conv2, *filter;
static GstPad *dbin_srcpad;
static gboolean linked = FALSE;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_warning (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);
      break;
    }
    case GST_MESSAGE_EOS:
      g_print ("Got EOS\n");
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }

  return TRUE;
}

static gint in_idle_probe = FALSE;

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *sinkpad, *srcpad;

  if (!g_atomic_int_compare_and_exchange (&in_idle_probe, FALSE, TRUE)) {
    g_print ("g_atomic\n");
    return GST_PAD_PROBE_OK;
  }
    
  /* Insert or remove filter? */
  if (conv2) {
    /* step1.断开pad连接 */
    sinkpad = gst_element_get_static_pad (conv2, "sink");
    gst_pad_unlink (dbin_srcpad, sinkpad);
    gst_object_unref (sinkpad);

    sinkpad = gst_element_get_static_pad (conv, "sink");
    srcpad = gst_element_get_static_pad (filter, "src");
    gst_pad_unlink (srcpad, sinkpad);
    gst_object_unref (srcpad);

    /* step2.移除元素 */
    gst_bin_remove (GST_BIN (pipeline), filter);
    gst_bin_remove (GST_BIN (pipeline), conv2);

    /* step3.设定元素状态 */
    gst_element_set_state (filter, GST_STATE_NULL);
    gst_element_set_state (conv2, GST_STATE_NULL);

    g_print ("GST_OBJECT_REFCOUNT_VALUE(conv2) = %d\n", GST_OBJECT_REFCOUNT_VALUE(conv2));
    g_print ("GST_OBJECT_REFCOUNT_VALUE(filter) = %d\n", GST_OBJECT_REFCOUNT_VALUE(filter));

    /* step4.解引用 */
    gst_object_unref (filter);
    gst_object_unref (conv2);

    filter = NULL;
    conv2 = NULL;

    if (gst_pad_link (dbin_srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link src with conv\n");
      gst_object_unref (sinkpad);
      g_main_loop_quit (loop);
    }
    gst_object_unref (sinkpad);
  } else {
    conv2 = gst_element_factory_make ("videoconvert", NULL);
    filter = gst_element_factory_make ("agingtv", NULL);

    gst_object_ref (conv2);
    gst_object_ref (filter);
    
    gst_bin_add_many (GST_BIN (pipeline), conv2, filter, NULL);

    gst_element_sync_state_with_parent (conv2);
    gst_element_sync_state_with_parent (filter);

    if (!gst_element_link (conv2, filter)) {
      g_printerr ("Failed to link conv2 with filter\n");
      g_main_loop_quit (loop);
    }

    sinkpad = gst_element_get_static_pad (conv, "sink");
    gst_pad_unlink (dbin_srcpad, sinkpad);
    gst_object_unref (sinkpad);

    sinkpad = gst_element_get_static_pad (conv2, "sink");
    if (gst_pad_link (dbin_srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link src with conv2\n");
      gst_object_unref (sinkpad);
      g_main_loop_quit (loop);
    }
    gst_object_unref (sinkpad);

    srcpad = gst_element_get_static_pad (filter, "src");
    sinkpad = gst_element_get_static_pad (conv, "sink");
    if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link filter with conv\n");
      gst_object_unref (srcpad);
      gst_object_unref (sinkpad);
      g_main_loop_quit (loop);
    }
    gst_object_unref (srcpad);
    gst_object_unref (sinkpad);
  }

  return GST_PAD_PROBE_REMOVE;
}

static gboolean
timeout_cb (gpointer user_data)
{
  in_idle_probe = FALSE;
  gst_pad_add_probe (dbin_srcpad, GST_PAD_PROBE_TYPE_IDLE, pad_probe_cb, NULL,
      NULL);

  return TRUE;
}

static void
pad_added_cb (GstElement * element, GstPad * pad, gpointer user_data)
{
  GstCaps *caps;
  GstStructure *s;
  const gchar *name;

  if (linked)
    return;

  caps = gst_pad_get_current_caps (pad);
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  if (strcmp (name, "video/x-raw") == 0) {
    GstPad *sinkpad;

    sinkpad = gst_element_get_static_pad (conv, "sink");
    if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link dbin with conv\n");
      g_main_loop_quit (loop);
    }
    gst_object_unref (sinkpad);

    dbin_srcpad = gst_object_ref (pad);

    g_timeout_add_seconds (2, timeout_cb, NULL);
    linked = TRUE;
  }

  gst_caps_unref (caps);
}

int
main (int argc, char **argv)
{
  GstBus *bus;

  gst_init (&argc, &argv);

  pipeline = gst_pipeline_new (NULL);
  src = gst_element_factory_make ("videotestsrc", NULL);
  dbin = gst_element_factory_make ("decodebin", NULL);
  conv = gst_element_factory_make ("videoconvert", NULL);
  scale = gst_element_factory_make ("videoscale", NULL);
  navseek = gst_element_factory_make ("navseek", NULL);
  queue = gst_element_factory_make ("queue", NULL);
  sink = gst_element_factory_make ("autovideosink", NULL);

  if (!pipeline || !src || !dbin || !conv || !scale || !navseek || !queue
      || !sink) {
    g_error ("Failed to create elements");
    return -2;
  }

  gst_bin_add_many (GST_BIN (pipeline), src, dbin, conv, scale, navseek, queue,
      sink, NULL);
  if (!gst_element_link_many (src, dbin, NULL)
      || !gst_element_link_many (conv, scale, navseek, queue, sink, NULL)) {
    g_error ("Failed to link elements");
    return -3;
  }

  g_signal_connect (dbin, "pad-added", G_CALLBACK (pad_added_cb), NULL);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), NULL);
  gst_object_unref (GST_OBJECT (bus));

  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_error ("Failed to go into PLAYING state");
    return -4;
  }

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_main_loop_unref (loop);

  if (dbin_srcpad)
    gst_object_unref (dbin_srcpad);
  gst_object_unref (pipeline);

  return 0;
}
