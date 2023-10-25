## 1 概述
&emsp;&emsp;BUS（总线）是一个简单的系统，拥有自己的线程机制将一个管道线程的消息分发到一个应用程序当中。总线的优势是：当使用GStreamer的时候，应用程序不需要线程识别，即便GStreamer已经被加载了多个线程。
&emsp;&emsp;每个管道默认包含一个总线，所以应用程序不需要再创建总线。应用程序只需要在总线上设置一个类型于对象的信号处理器的消息处理器。当主循环运行的时候，总线将会轮询这个消息处理器是否有新的消息，当消息采集到后，总线将呼叫相应的回调函数来完成任务。
&emsp;&emsp;上一章节官方提供的例程使用的gst阻塞函数，学习BUS（总线）后发现，可以运行GLib/Gtk主循环（也可以运行默认的GLib主循环），然后使用侦听器对总线进行侦听。
## 2 代码
&emsp;&emsp;需要注意的是，可以通过gst_bus_remove_watch和回调函数返回FALSE删除监听。这个bus监听是一个引用，为了保持内存安全，设置监听后，需要gst_object_unref这个bus监听。

```c
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
```

[参考1 Gstreamer Bus例程](https://www.cnblogs.com/xuyh/p/4562999.html)
[参考2 官方手册Gbus](https://gstreamer.freedesktop.org/documentation/gstreamer/gstbus.html?gi-language=c#GstBus)