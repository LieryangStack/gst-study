# 9 Building a Test Application
通常，你会希望在尽可能小的设置中测试新编写的插件。通常，gst-launch-1.0是测试插件的第一步。如果没有将插件安装在GStreamer搜索的目录中，则需要设置插件路径。将GST_PLUGIN_PATH设置为包含插件的目录，或者使用命令行选项`--gst-plugin-path`。如果你的插件基于gst-plugin模板，那么它看起来类似于`gst-launch-1.0 --gst-plugin-path=$HOME/gst-template/gst-plugin/src/.libs TESTPIPELINE`，然而库测试管道，您通常需要比gst-launch-1.0提供的更多的测试功能，例如查找、事件、交互等。编写自己的小测试程序是实现这一点的最简单方法。本节简单地解释了如何做到这一点。要了解完整的应用程序开发指南，请参阅应用程序开发手册。

开始时，你需要通过调用gst_init()初始化GStreamer核心库。您也可以调用gst_init_get_option_group()，它将返回一个指向GOptionGroup的指针。然后您可以使用GOption来处理初始化，这将完成GStreamer的初始化。

可以使用gst_element_factory_make()创建元素，其中第一个参数是要创建的元素类型，第二个参数是自由格式的名称。最后的示例使用了一个简单的filesource - decoder -声卡输出管道，但如果有必要，您可以使用特定的调试元素。例如，可以在管道中间使用标识元素作为数据到应用的发送器。这可以用于检查测试应用程序中的数据是否有错误行为或是否正确。此外，还可以在管道的末尾使用fakesink元素将数据转储到stdout(为此，请将dump属性设置为TRUE)。最后，你可以使用`valgrind`检查内存错误。

在链接期间，您的测试应用程序可以使用过滤的caps作为一种方式来驱动特定类型的数据与元素之间的传输。这是检查元素中多种输入和输出类型的一种非常简单有效的方法。

请注意，在运行期间，您应该至少侦听总线和/或插件/元素上的“error”和“eos”消息，以检查是否正确处理此问题。此外，还应该向管道中添加事件，并确保插件正确地处理这些事件(时钟、内部缓存等)。

永远不要忘记清理插件或测试应用程序中的内存。当变为NULL状态时，你的元素应该清理分配的内存和缓存。此外，它应该关闭所有可能的支持库的引用。您的应用程序应该unref()管道并确保它不会崩溃。

```c
#include <gst/gst.h>

static gboolean
bus_call (GstBus     *bus,
      GstMessage *msg,
      gpointer    data)
{
  GMainLoop *loop = data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End-of-stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR: {
      gchar *debug = NULL;
      GError *err = NULL;

      gst_message_parse_error (msg, &err, &debug);

      g_print ("Error: %s\n", err->message);
      g_error_free (err);

      if (debug) {
        g_print ("Debug details: %s\n", debug);
        g_free (debug);
      }

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

gint
main (gint   argc,
      gchar *argv[])
{
  GstStateChangeReturn ret;
  GstElement *pipeline, *filesrc, *decoder, *filter, *sink;
  GstElement *convert1, *convert2, *resample;
  GMainLoop *loop;
  GstBus *bus;
  guint watch_id;

  /* initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);
  if (argc != 2) {
    g_print ("Usage: %s <mp3 filename>\n", argv[0]);
    return 01;
  }

  /* create elements */
  pipeline = gst_pipeline_new ("my_pipeline");

  /* watch for messages on the pipeline's bus (note that this will only
   * work like this when a GLib main loop is running) */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  filesrc  = gst_element_factory_make ("filesrc", "my_filesource");
  decoder  = gst_element_factory_make ("mad", "my_decoder");

  /* putting an audioconvert element here to convert the output of the
   * decoder into a format that my_filter can handle (we are assuming it
   * will handle any sample rate here though) */
  convert1 = gst_element_factory_make ("audioconvert", "audioconvert1");

  /* use "identity" here for a filter that does nothing */
  filter   = gst_element_factory_make ("my_filter", "my_filter");

  /* there should always be audioconvert and audioresample elements before
   * the audio sink, since the capabilities of the audio sink usually vary
   * depending on the environment (output used, sound card, driver etc.) */
  convert2 = gst_element_factory_make ("audioconvert", "audioconvert2");
  resample = gst_element_factory_make ("audioresample", "audioresample");
  sink     = gst_element_factory_make ("pulsesink", "audiosink");

  if (!sink || !decoder) {
    g_print ("Decoder or output could not be found - check your install\n");
    return -1;
  } else if (!convert1 || !convert2 || !resample) {
    g_print ("Could not create audioconvert or audioresample element, "
             "check your installation\n");
    return -1;
  } else if (!filter) {
    g_print ("Your self-written filter could not be found. Make sure it "
             "is installed correctly in $(libdir)/gstreamer-1.0/ or "
             "~/.gstreamer-1.0/plugins/ and that gst-inspect-1.0 lists it. "
             "If it doesn't, check with 'GST_DEBUG=*:2 gst-inspect-1.0' for "
             "the reason why it is not being loaded.");
    return -1;
  }

  g_object_set (G_OBJECT (filesrc), "location", argv[1], NULL);

  gst_bin_add_many (GST_BIN (pipeline), filesrc, decoder, convert1, filter,
                    convert2, resample, sink, NULL);

  /* link everything together */
  if (!gst_element_link_many (filesrc, decoder, convert1, filter, convert2,
                              resample, sink, NULL)) {
    g_print ("Failed to link one or more elements!\n");
    return -1;
  }

  /* run */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GstMessage *msg;

    g_print ("Failed to start up pipeline!\n");

    /* check if there is an error message with details on the bus */
    msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
    if (msg) {
      GError *err = NULL;

      gst_message_parse_error (msg, &err, NULL);
      g_print ("ERROR: %s\n", err->message);
      g_error_free (err);
      gst_message_unref (msg);
    }
    return -1;
  }

  g_main_loop_run (loop);

  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_source_remove (watch_id);
  g_main_loop_unref (loop);

  return 0;
}

```