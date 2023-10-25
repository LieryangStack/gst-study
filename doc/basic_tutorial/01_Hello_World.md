## 写在开头
&emsp;&emsp;这些基本教程讲述了一般性话题，这些教程可以帮助你理解其他教程。
## 1 代码

```c
#include <gst/gst.h>

int
main (int argc, char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;
  gboolean terminate = FALSE;

  /* Initialize GStreamer */
  /* 这是必须第一个执行的Gstreamer命令
   * 初始化所有内部结果
   * 检查可用的插件
   * 执行外部传入的命令行选项 argc argv[]
   */
  gst_init (&argc, &argv);

  /* Build the pipeline */
  /* 创建和链接所有播放媒体所必须的elements*/
  pipeline =
      gst_parse_launch
      ("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  /* 设置流管播放状态 */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  
  /* Wait until error or EOS */
  /* 获得pipeline总线的bus */
  bus = gst_element_get_bus (pipeline);
  /* 阻塞一直等待一个错误或者播放结束 */
  msg =
    gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, 
    GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
#if 0
 msg =
    gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, 
    GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  do{
    msg = 
      gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ANY);
    if(msg != NULL)
    {
      GstState old_state, new_state, pending_state;
      switch(GST_MESSAGE_TYPE (msg)){
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          //if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) 
            gst_message_parse_state_changed (msg, &old_state, &new_state,
              &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
              gst_element_state_get_name (old_state),
              gst_element_state_get_name (new_state));
            
            break;
        default:
            g_printerr ("%x\n", GST_MESSAGE_TYPE (msg));
            break;
      }
    }
  }while(!terminate);
#endif
  /* Free resources */
  if(msg != NULL)
    /* gst_bus_timed_pop_filtered 返回一个message,这个需要释放资源 */
    gst_message_unref (msg);
  /* gst_element_get_bus 会对总线增加一个引用， 所以也需要释放资源 */
  gst_object_unref (bus);
  /* pipeline 设置为NULL会自动释放掉所有的资源 */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  /* 最后释放掉pipeline自身 */
  gst_object_unref (pipeline);
  return 0;
}

```
## 2 代码分析
&emsp;&emsp;每个GStreamer程序必须的命令，该函数用来初始化所有内vu结构，检查可用的插件，执行任何用于该程序的命令选项。
```c
gst_init(&argc, &argv);
```
### 2.2 gst_parse_launch( )
&emsp;&emsp;主要理解：**playbin**和**gst_parse_launch( )** ，流管继承关系如下所示，可以通过 gst-inspect-1.0 playbin 命令查看更多关于playbin信息。
&emsp;&emsp;大部分核心功能都是继承于GstObject和GstMiniObject。
```bash
GObject
 +----GInitiallyUnowned
         +-------GstObject
              +-------GstBus
              +-------GstPad
              +-------GstElement
                 +-------GstBin
					+-------GstPipeline
					   +-------GstPlayBin
```

```bash
GstMiniObject
  +------GstMessage
  +------GstCaps
```
&emsp;&emsp;创建一个playbin管道，其中已经包含了管道所必须的元素，另一个GError 参数设置NULL。
```c
/* 使用http */
pipeline =
      gst_parse_launch
      ("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);
/* 使用file */
pipeline =
      gst_parse_launch
      ("playbin uri=file:///home/lieryang/Desktop/gst-docs/examples/tutorials/sintel_trailer-480p.webm",
      NULL);
```
### 2.3 gst_element_set_state( )
&emsp;&emsp;每个GStreamer元素都有一个关联的状态，具体状态参数会在下将介绍。
### 2.4 总线消息
&emsp;&emsp;每一个管道默认包含一个总线，所以一个管道的元素共有一个bus
&emsp;&emsp;gst_element_get_bus获得pipeline的消息总线;使用gst_bus_timed_pop_filtered的第二个参数时间设置成None，一直阻塞等到参数三设定的消息出现。**types**是个枚举类型，具体可以参考函数头文件。
```c
GstBus *
gst_element_get_bus(GstElement * element);
GstMessage *
gst_bus_timed_pop_filtered(GstBus * bus, 
                           GstClockTime timeout, 
                           GstMessageType types);
```
### 2.5 资源释放

```c
  /* Free resources */
  if(msg != NULL)
    /* gst_bus_timed_pop_filtered 返回一个message,这个需要释放资源 */
    gst_message_unref (msg);
  /* gst_element_get_bus 会对总线增加一个引用， 所以也需要释放资源 */
  gst_object_unref (bus);
  /* pipeline 设置为NULL会自动释放掉所有的资源 */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  /* 最后释放掉pipeline自身 */
  gst_object_unref (pipeline);
  g_main_loop_unref(loop);
```
## 3 本章节函数总结

```c
 /*1.GStreamer初始化 */
void gst_init (int *argc, char **argv[]);

/* 2.自动创建流管 */
GstElement* gst_parse_launch(const gchar * pipeline_description,
                             GError ** error);
/*3.设置流管状态 */
GstStateChangeReturn  gst_element_set_state (GstElement *element,
                                             GstState state);
/*4.获得流管bus*/
GstBus * gst_element_get_bus (GstElement * element);
/*5.阻塞等待消息到来、可设定阻塞时间长短*/
GstMessage * gst_bus_timed_pop_filtered (GstBus * bus, 
                                         GstClockTime timeout, 
                                         GstMessageType types);
/*6.资源释放*/
gst_message_unref (GstMessage * msg)
GstStateChangeReturn gst_element_set_state (GstElement *element, 
                                            GstState state);
void gst_object_unref (gpointer object);
```

[参考1：Basic tutorial 1: Hello world!](https://gstreamer.freedesktop.org/documentation/tutorials/basic/hello-world.html?gi-language=c)