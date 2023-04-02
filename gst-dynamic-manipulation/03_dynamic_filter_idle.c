/**
 * 由于文件01_changing_element中，删除元素后，引用数不等于0（但是不能再次解引用），
 * 我决定用示例02（元素引用计数为0）的空闲监听尝试一下
 * 空闲监听，引用计数等0
*/

#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category

static GstPad *conv_before_src_pad;
static GstElement *conv_before;
static GstElement *conv_after;
static GstElement *effect_a;
static GstElement *effect_b;
static GstElement *pipeline;

static gint probe = FALSE;
static gulong block_probe_id = 0;

static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data) {

  /*if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;
*/
  if (!g_atomic_int_compare_and_exchange (&probe, FALSE, TRUE)) {
    g_print ("g_atomic\n");
    return GST_PAD_PROBE_OK;
  }

  if (effect_a) {
    gst_element_unlink_many (conv_after, effect_a, conv_before, NULL);
    gst_bin_remove (GST_BIN (pipeline), effect_a);
    gst_element_set_state (effect_a, GST_STATE_NULL);
    g_print ("GST_OBJECT_REFCOUNT_VALUE(effect_a) = %d\n", GST_OBJECT_REFCOUNT_VALUE(effect_a));
    gst_clear_object(&effect_a);

    /**
     * 经过 gst_element_set_state (effect_a, GST_STATE_NULL);
     *     gst_bin_remove (GST_BIN (pipeline), effect_a); 操作
     * 依据我对g_object对象的理解和查看引用总数、以及GST_IS_ELEMENT (effect_a)
     * 发现引用总数并不为0,而且effect_a是元素对象
     * @@@但是@@@并不能再次解引用（再次解引用会引起系统崩溃），可能gstreamer通过上面两步已经把所有内存释放
     * 
     * 通过其他示例，我知道如何解决这个问题：
     * 元素创建完成之后进行 ref (造成这个问题的原因可能是因为浮点引用)
     * 删除元素的方法：
     * 1.断开链接
     * 2.移除元素
     * 3.设定NULL状态
     * 4.解引用
     * 
     * 引用数稳定在1,不能再次解引用，再次解应用内存错误 （为什么02_dynamic_filter引用计数为0？？？）
     * 
    */

    /* 更换元素 */
    effect_b = gst_element_factory_make ("gaussianblur", "effect_b");
    gst_object_ref (effect_b);
    gst_bin_add (GST_BIN (pipeline), effect_b);
    gst_element_sync_state_with_parent (effect_b);
    gst_element_link_many (conv_before, effect_b, conv_after, NULL);
    g_print ("effect_a ->  effect_b\n");
  }
  else if (effect_b){
    gst_element_unlink_many (conv_after, effect_b, conv_before, NULL);
    gst_bin_remove (GST_BIN (pipeline), effect_b);
    gst_element_set_state (effect_b, GST_STATE_NULL);
    g_print ("GST_OBJECT_REFCOUNT_VALUE(effect_b) = %d\n", GST_OBJECT_REFCOUNT_VALUE(effect_b));
    gst_clear_object(&effect_b);

    effect_a = gst_element_factory_make ("shagadelictv", "effect_a");
    gst_object_ref (effect_a);
    gst_bin_add (GST_BIN (pipeline), effect_a);
    gst_element_sync_state_with_parent (effect_a);
    gst_element_link_many (conv_before, effect_a, conv_after, NULL);
    g_print ("effect_b ->  effect_a\n");
  }

  return GST_PAD_PROBE_REMOVE;
}

static gboolean
timeout_cb (gpointer user_data)
{
  probe = FALSE;

  block_probe_id = gst_pad_add_probe (conv_before_src_pad, GST_PAD_PROBE_TYPE_IDLE,
                                         event_probe_cb, user_data, NULL);

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
  /* gst初始化后初始 , 0日志字符表示无颜色输出， 1表示有颜色输出*/
  GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");
  
  pipeline = gst_pipeline_new ("pipeline");

  videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  queue_1 = gst_element_factory_make ("queue", "queue_1");
  conv_before = gst_element_factory_make ("videoconvert", "conv_before");
  effect_a = gst_element_factory_make ("shagadelictv", "effect_a");
  conv_after = gst_element_factory_make ("videoconvert", "conv_after");
  queue_2 = gst_element_factory_make ("queue", NULL);
  sink = gst_element_factory_make ("ximagesink", NULL);

  gst_object_ref (effect_a);
  
  gst_util_set_object_arg (G_OBJECT (capsfilter), "caps",
    "video/x-raw, width=320, height=240, "
    "format={ I420, YV12, YUY2, UYVY, AYUV, Y41B, Y42B, "
    "YVYU, Y444, v210, v216, NV12, NV21, UYVP, A420, YUV9, YVU9, IYU1 }");
  conv_before_src_pad = gst_element_get_static_pad (conv_before, "src");

  g_object_set (videotestsrc, "is-live", TRUE, NULL);
  g_object_set (sink, "sync", FALSE, NULL);

  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, capsfilter, queue_1, conv_before, effect_a,
      conv_after, queue_2, sink, NULL);

  gst_element_link_many (videotestsrc, capsfilter, queue_1, conv_before, effect_a,
      conv_after, queue_2, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  loop = g_main_loop_new (NULL, FALSE);

  gst_bus_add_watch (GST_ELEMENT_BUS (pipeline), bus_cb, loop);

  g_timeout_add_seconds (1, timeout_cb, loop);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}