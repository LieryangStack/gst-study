/**
 * Test if idle probes are blocking or not.
 * 
 * Build dynamic pipeline made of a videotestsrc and an autovideosink, 
 * and add/remove a dummy idle probe (print a message) every 5 seconds.
 * 
 * Probes of type GST_PAD_PROBE_TYPE_IDLE are described as blocking
 * in the design documents [1] and the reference documentation [2],
 * so that they block the pad until the probe is removed.
 * But it seems that they just block the pad during the execution of the 
 * callback, and data flows through the pad after the callback has finished.
 * 
 * If the intended behavior is the one described in the documentation,
 * then this is a bug probably caused by the following lines in function
 * `do_probe_callbacks` (gst/gstpad.c):
 *
 *    is_block =
 *        (info->type & GST_PAD_PROBE_TYPE_BLOCK) == GST_PAD_PROBE_TYPE_BLOCK;
 *
 *    if (is_block && PROBE_TYPE_IS_SERIALIZED (info)) {
 *      if (do_pad_idle_probe_wait (pad) == GST_FLOW_FLUSHING)
 *        goto flushing;
 *    }
 * 
 * where `is_block` should be computed as:
 * 
 *    is_block = type & GST_PAD_PROBE_TYPE_BLOCKING;
 * 
 * Otherwise, the documentation should state that an idle probe will block
 * the pad during the execution of the callback, which will be triggered
 * each time the pad becomes idle (that is: after dealing with some data,
 * and also including the case where the pad is idle at the insertion of the
 * probe).
 * 
 *   [1]: http://cgit.freedesktop.org/gstreamer/gstreamer/tree/docs/design/part-probes.txt
 *   [2]: http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstPad.html
 */

#include <gst/gst.h>
#include <stdlib.h>

static GMainLoop * loop;
static GstElement * pipeline;
static GstElement * source, * sink;
static GstBus * bus;
static gulong pad_probe_id = 0;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
    GError * error;
    gchar * name, * debug;

    name = GST_MESSAGE_SRC_NAME(message);
    error = NULL;
    debug = NULL;
    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error (message, &error, &debug);
            g_printerr ("%s: from element %s: %s (%s)\n", "ERROR", name,
                        error->message, debug ? debug : "no debug info");
            g_error_free (error);
            g_free (debug);
            g_main_loop_quit (loop);
            break;
        case GST_MESSAGE_WARNING:
            gst_message_parse_warning (message, &error, &debug);
            g_printerr ("%s: from element %s: %s (%s)\n", "WARNING", name,
                        error->message,  debug ? debug : "no debug info");
            g_error_free (error);
            g_free (debug);
            break;
        case GST_MESSAGE_EOS:
            g_print ("Got EOS\n");
            g_main_loop_quit (loop);
            break;
        default:
            break;
  }
  return TRUE;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
    g_print("[%ld] running probe callback\n", g_get_real_time());
    for (int i = 0; i < 9999; i++) {
      g_print("%d ", i);
    }
    g_print("\n");
    return GST_PAD_PROBE_OK;
}

static gboolean
timeout_cb (gpointer user_data)
{
    GstPad * pad = gst_element_get_static_pad(source, "src");
    if (pad_probe_id == 0) {
        pad_probe_id = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_IDLE,
                                         pad_probe_cb, NULL, NULL);
    } else {
        gst_pad_remove_probe(pad, pad_probe_id);
        pad_probe_id = 0;
    }
    return TRUE;
}

int
main (int argc, char **argv)
{
  gst_init (&argc, &argv);

  pipeline = gst_pipeline_new (NULL);
  source = gst_element_factory_make ("videotestsrc", NULL);
  sink   = gst_element_factory_make ("autovideosink", NULL);

  if (! pipeline || ! source || ! sink) {
    g_printerr("Could not create elements\n");
    return EXIT_FAILURE;
  }

  g_object_set(source, "pattern", 18, NULL);
  
  gst_bin_add_many (GST_BIN(pipeline), source, sink, NULL);
  if (!gst_element_link_many (source, sink, NULL)) {
    g_printerr("Could not link elements\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), NULL);
  gst_object_unref (GST_OBJECT (bus));
  
  g_timeout_add_seconds(3, timeout_cb, NULL);

  if (gst_element_set_state (pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Failed to go into PLAYING state\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  
  gst_object_unref(pipeline);

  g_main_loop_unref (loop);

  return EXIT_SUCCESS;
}