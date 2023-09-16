/**
 * 监听总线消息
*/

#include<gst/gst.h>

static GMainLoop *loop;
static gboolean
my_bus_callback(GstBus *bus, GstMessage *message, gpointer data);


gint
main(gint argc, gchar *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;
  GError *error = NULL;

  gst_init(&argc, &argv);
  
  pipeline = gst_parse_launch
            ("playbin uri=file:///home/lieryang/Desktop/gst-study/sample-480p.webm", &error);
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  gst_bus_add_watch(bus, my_bus_callback, NULL);
  
  gst_object_unref(bus);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  
  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
   
  return FALSE;
}

static gboolean
my_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
  gchar *name; 
  name = gst_object_get_name(message->src);
  g_print("%d : Got %s message,the message is from %s\n", 
          message->seqnum,
          GST_MESSAGE_TYPE_NAME(message),
          name);
  
  switch (GST_MESSAGE_TYPE(message))
  {
    case GST_MESSAGE_EOS:
      g_main_loop_quit(loop);
      break;
    case GST_MESSAGE_TAG:/*元数据在数据流中被找到*/
      g_print("******TAG-FIND********\n");
      break;
    default:
      break;
  }
  gst_bus_post(bus, message);

  /*如果返回False，会自动删除监听*/
  return TRUE;
}
