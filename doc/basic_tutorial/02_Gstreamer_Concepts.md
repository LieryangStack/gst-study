## 1 目标
&emsp;&emsp;上一篇教程展示了如何自动构建管道。现在，我们将通过实例化每个元素并将它们链接到一起来手动构建一个管道。在这个过程中，我们会学到:

 - 什么是GStreamer元素以及如何创建一个元素。

 - 如何将每个元素相互连接。

 - 如何自定义元素的行为。

 - 如何监视总线的错误情况并从GStreamer message中提取信息。
## 2 代码

```c
#include <gst/gst.h>

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
  if (gst_element_link (source, sink) != TRUE) {
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

```
## 2 概述
这些元素是GStreamer的基本结构块。它们处理数据,当数据从source element(数据生产者)流向sink elements(数据消费者)时，通过filter elements。

![Figure 1.Example pipeline](https://img-blog.csdnimg.cn/764a1b657c304ac88feb09ca76db4f10.png)
## 3 元素创建
我们将跳过GStreamer初始化，因为它与前面的教程相同:
```c
/* Initialize GStreamer */
gst_init (&argc, &argv);
```
&emsp;&emsp;如这段代码所示，可以使用gst_element_factory_make()创建新元素。第一个参数是要创建的元素类型(基础教程14:Handy elements展示了一些常见类型，基础教程10:GStreamer tools展示了如何获取所有可用类型的列表)。第二个参数是我们想要赋值这个特定实例的名称。如果您没有保留该实例的指针，可以通过命名元素去检索它们，使用他们的名称更多是在debug输出的时候。但是，如果您为名称传递NULL, GStreamer将为您提供一个唯一的名称。

&emsp;&emsp;在本教程中，我们创建了两个元素:videotestsrc和autovideosink。没有filter elements。因此，管道看起来如下所示:
![在这里插入图片描述](https://img-blog.csdnimg.cn/541eaf63ea7f4106ab45a334770fd497.png)
&emsp;&emsp;Videotestsrc是一个source element (它生成数据)，它创建一个测试视频模式。此元素用于调试(和教程)，在实际应用程序中通常找不到。

&emsp;&emsp;Autovideosink是一个sink element(它消耗数据)，它在一个窗口中显示它接收到的图像。根据操作系统的不同，有几种不同功能的视频接收器。
&emsp;&emsp;Autovideosink自动选择并实例化最好的，因此您不必担心细节，并且您的代码更独立于平台(受平台影响不大)。
## 4 管道创建
```c
 /* Create the elements */
 source = gst_element_factory_make ("videotestsrc", "source");
```
GStreamer中的所有元素通常必须包含在pipeline中才能使用，因为它负责一些时钟和消息传递函数。我们使用gst_pipeline_new()创建管道。

```c
  g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
  if (gst_element_link (source, sink) != TRUE) {
```

管道是一种特殊类型的bin，它也是元素，只不过通常被用来包含其它元素。因此，所有的方法适用于bin，也适用于pipeline。

在本例中，我们调用gst_bin_add_many()将元素添加到pipeline(注意转换)。这个函数接受一个要添加的元素列表，以NULL结尾。可以使用gst_bin_add()添加单个元素。

然而，这些元素之间还没有联系。为此，我们需要使用gst_element_link()。它的第一个参数是source，第二个参数是一个destination。顺序很重要，因为必须按照数据流(即从源元素到接收元素)建立链接。请记住，只有位于同一bin中的元素才能链接在一起，所以在尝试链接它们之前，请记住将它们添加到管道中!

## 5 属性
GStreamer元素都是一种特殊的GObject，它是提供属性功能的实体。

大多数GStreamer元素都有可定制的属性:可以修改属性以改变元素的行为(可写属性)，或者查询属性以了解元素的内部状态(可读属性)。

属性从g_object_get()读取，用g_object_set()写入。

g_object_set()接受以null结束的属性名称、属性值对列表，因此可以一次性更改多个属性。


回到上面的例子，

```c
 /* Modify the source's properties */
  g_object_set (source, "pattern", 0, NULL);
```

上面的代码行更改了videotestsrc的“pattern”属性，该属性控制元素输出的测试视频的类型。尝试不同的价值!

可以使用基本教程10:GStreamer工具中描述的gst-inspect-1.0工具，或者在该元素的文档(这里是videotestsrc的例子)中找到元素公开的所有属性的名称和可能的值。

## 6 错误检查
在这一点上，我们已经构建和设置了整个管道，教程的其余部分与前一个非常相似，但我们将添加更多的错误检查:

```c
  /* Modify the source's properties */
  g_object_set (source, "pattern", 0, NULL);

  /* Start playing */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {

```
我们调用gst_element_set_state()，但这次我们检查它的返回值是否有错误。改变状态是一个微妙的过程，在基本教程3:动态管道中给出了更多细节。

```c
 gst_object_unref (pipeline);
    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Parse message */
  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
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

```
gst_bus_timed_pop_filtered()等待执行结束并返回一个我们之前忽略的GstMessage。我们要求gst_bus_timed_pop_filtered()在GStreamer遇到错误条件或EOS时返回，因此我们需要检查发生了哪个错误，并在屏幕上打印一条消息(您的应用程序可能想要进行更复杂的操作)。

GstMessage是一种非常通用的结构，几乎可以传递任何类型的信息。幸运的是，GStreamer为每种类型的消息提供了一系列解析函数。

在这种情况下，一旦我们知道消息包含一个错误(通过使用GST_MESSAGE_TYPE()宏)，我们可以使用gst_message_parse_error()，它返回一个GLib GError错误结构体和一个对于调试有用的字符串。请看代码，看看它们是如何被使用和释放的。

## 7 GStreamer bus

此时，有必要更正式地介绍GStreamer总线。它是负责将元素生成的GstMessages按顺序传递给应用程序并传递给应用程序线程的对象。最后一点很重要，因为实际的媒体流是在应用程序之外的另一个线程中完成的。

可以使用gst_bus_timed_pop_filtered()及其兄弟函数从总线中同步提取消息，也可以使用信号(将在下一篇教程中介绍)从总线中异步提取消息。你的应用程序应该时刻关注总线，以便在出现错误和其他与播放相关的问题时得到通知。

其余的代码是释放序列，这与基本教程1:Hello world!相同。
## 8 练习

如果你想练习，可以试试这个练习:在管道的source和sink之间添加一个视频filter元素。使用vertigotv可以获得更好的效果。您需要创建它，将其添加到管道中，并将其与其他元素链接。

根据你的平台和可用的插件，你可能会得到一个“协商”错误，因为sink不理解filter生成的是什么(有关协商的更多信息，请参阅基本教程6:媒体格式和Pad功能)。在本例中，尝试在filter之后添加一个名为videoconvert的元素(也就是构建一个包含4个元素的管道)。更多关于视频转换的基本教程14:方便的元素)。

## 9 函数总结
```c
/* 1.元素创建 */
GstElement* gst_element_factory_make (const gchar *factoryname,
                                      const gchar *name);
                                      
/*2.元素name获取宏*/
GST_ELEMENT_NAME(elem)

/*3.新建管道*/
GstElement* gst_pipeline_new(const gchar *name);

/*4.元素添加进管道*/
void gst_bin_add_many (GstBin *bin, GstElement *element_1, ...);

/*5.类型改变宏*/
GST_BIN(obj)

/*6.管道元素连接*/
gboolean gst_element_link_many (GstElement *element_1,
                                GstElement *element_2, ...)

/*7.Gobject对象多个属性设置*/
void g_object_set (gpointer	       object,
				   const gchar    *first_property_name,
					                               ...);
/*8.获得Message信息类型宏 */
GST_MESSAGE_TYPE（message）
/*9.读出message中包含的具体错误信息*/
void gst_message_parse_error (GstMessage *message, 
                              GError **gerror, 
                              gchar **debug);

```

这是专门介绍基本GStreamer概念的两篇教程中的第一篇。接下来是第二个。

请记住，在这个页面的附件中，您应该可以找到本教程的完整源代码和构建它所需的任何附件文件。

很高兴在这里见到你，希望很快见到你!
