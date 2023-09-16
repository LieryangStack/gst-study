#include <gst/gst.h>

static gboolean
link_elemets_with_filter(GstElement *element1, GstElement *element2);

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  
  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  /* 新的element的建立可以使用gst_element_factory_make
   * Param 1:要创建的element的类型（这个类型必须是Gstreamer有的插件）
   * Param 2:自己命名一个element的名字，这个名字可以是任意的。
   *         如果设置成NULL，Gstreamer会自动创建一个名字
   *         这个名字常用在调试过程中
   */
  /* videotestsrc常用在调试中，很少用于实际的应用 */
  source = gst_element_factory_make ("videotestsrc", "lieryang");
  /*   会在一个窗口显示收到的图像。在不同的操作系统中，会存在多个的video sink，
   * autovideosink会自动选择一个最合适的
   */

  /*获得元素的名字*/
  g_print ("Element name is '%s'\n", GST_ELEMENT_NAME(source));

  sink = gst_element_factory_make ("autovideosink", "sink");

  /* Create the empty pipeline */
  /* 因为要统一处理始终和一些信息
   * 所以Element使用之前包含到pipeline
   */
  pipeline = gst_pipeline_new ("test-pipeline");

  /* 判断pipeline和Element是否创建成功 */
  if (!pipeline || !source || !sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  /* pipeline就是一个特定类型的可以包含其他element的bin
   * NULL表示停止
   */
  gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
  /* gst_bin_add_many 添加element还没有连接起来
   * 只有在同一个bin里面的element才能连接起来，所以一定要把element加入到pipeline
   * Param 1：是源
   * Param 2：是目标
   */
  if (link_elemets_with_filter (source, sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  /* Modify the source's properties */
  /* 绝大部分GSreamer elements有可以定制化的属性
   * 接受一个用NULL结束的属性名称/属性值的组成的对，所以可以一次同时修改多项属性
   * gst-inspect-1.0命令可以查看element属性
   */
  g_object_set (source, "pattern", 0, NULL);

  /* Start playing */
  /* 检查状态转换的返回值 
   */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  /* 消息在bus总线上，可以用gst_bus_timed_pop_filtered抓出出错信息和播放相关信息 */
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Parse message */
  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
        /* 解析错误信息 */
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s\n",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        break;
      case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        break; 
      default:
        /* We should not reach here because we only asked for ERRORs and EOS */
        g_printerr ("Unexpected message received.\n");
        break;
    }
    gst_message_unref (msg);
  }

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  return 0;
}

static gboolean
link_elemets_with_filter(GstElement *element1, GstElement *element2){
  gboolean link_ok;
  GstCaps *caps;

  caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "I420",
            "width", G_TYPE_INT, 416,
            "height", G_TYPE_INT, 416,
            "framerate", GST_TYPE_FRACTION, 100, 1,
            NULL);
  link_ok = gst_element_link_filtered(element1, element2, caps);
  
  gst_caps_unref(caps);

  if(!link_ok){
    g_warning("Failed to link element1 and element2");
  }

  return link_ok;
}