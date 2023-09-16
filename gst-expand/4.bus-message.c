#include<gst/gst.h>

static GMainLoop *loop;
static void eos_cb 
(GstBus *bus, GstMessage *msg, GstElement *pipeline);


gint
main(gint argc, gchar *argv[])
{
  GstElement *pipeline, *sink;
  GstBus *bus;
  GstMessage *msg;
  GError *error = NULL;

  gst_init(&argc, &argv);
  
  sink = gst_element_factory_make ("xvimagesink", NULL); g_assert(sink);
  pipeline = gst_parse_launch
            ("playbin uri=file:///home/lieryang/Desktop/gst-study/sample-480p.webm", &error);
  g_object_set (pipeline, "video-sink", sink, NULL);
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  gst_bus_add_signal_watch(bus);
  g_signal_connect(G_OBJECT(bus), "message::state-changed", G_CALLBACK(eos_cb), pipeline);
  
  gst_object_unref(bus);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  
  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
   
  return FALSE;
}

static void eos_cb (GstBus *bus, GstMessage *msg, GstElement *pipeline) {
  g_print ("End-Of-Stream reached.\n");
  //gst_element_set_state (pipeline, GST_STATE_READY);
}
