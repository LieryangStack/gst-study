#include <gst/gst.h>
#include<glib-object.h>

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *sink, *src;
  GstBus *bus;
  GstMessage *msg;
  gboolean terminate = FALSE;

  gint width, height;

  /* 设定环境变量值 GST_DEBUG_DUMP_DOT_DIR = /home/lieryang/Desktop/gst-study/basic-tutorial/graphs/ 
   * 一定要放在gst_init初始化前面
   * 这个环境变量告诉gst要生成pipeline graphs
   */
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", "/home/lieryang/Desktop/gst-study/basic-tutorial/graphs/", TRUE);

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
      ("playbin uri=file:///home/lieryang/Desktop/gst-study/sample-480p.webm",
      NULL);

  /* Start playing */
  /* 设置流管播放状态 */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
  
  /* Wait until error or EOS */
  /* 获得pipeline总线的bus */
  bus = gst_element_get_bus (pipeline);
#if 0
  /* 阻塞一直等待一个错误或者播放结束 */
  msg =
  gst_bus_timed_pop_filtered (bus, 3 * GST_SECOND, 
  GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  msg =
  gst_bus_timed_pop_filtered (bus, GST_CLOCK_STIME_NONE, 
  GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
#endif
#if 1
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
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_ERROR:
          terminate = TRUE;
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
  /* 设定到PAUSE状态，保存pipeline graphs */
  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-PLAING-PAUSE");
  /* pipeline 设置为NULL会自动释放掉所有的资源 */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  /* 最后释放掉pipeline自身 */
  gst_object_unref (pipeline);

  return 0;
 
}
