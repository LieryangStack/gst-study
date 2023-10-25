## 目标
本教程展示了使用GStreamer所需的其余基本概念，它允许在信息可用时“动态”构建管道，而不是在应用程序开始时定义一个完整的管道。

在本教程之后，您将具备开始playback tutorial所需的知识。这里回顾的要点是:

 - 在连接元素的时候，如果添加更好的控制。
 - 如何收到有趣事件的通知，以便及时作出反应。
 - 元素可能处于的各种状态。
 
## 介绍
正如你将要看到的，本教程中的管道在设置为播放状态之前还没有完全构建好。这没问题。如果我们不采取进一步的措施，数据将到达管道的末端，管道将产生错误信息并停止。但我们将采取进一步行动……

在这个例子中，我们打开了一个多路复用multiplexed(或缩写muxed)的文件，音频和视频一起存储在一个容器文件中。负责打开容器的元件称为解模器(demuxer)，容器格式的例子有Matroska (MKV)、Quick Time (QT、MOV)、Ogg或高级系统格式(ASF、WMV、WMA)。

如果一个容器嵌入了多个流(例如，一个视频和两个音频轨道)，demuxer将把它们分开，并通过不同的输出端口输出它们。通过这种方式，可以在管道中创建不同的分支，处理不同类型的数据。

GStreamer元素相互通信所通过的端口称为pad (GstPad)。存在sink pad(数据通过它进入元素)和source pad(数据通过它退出元素)。很自然地，source元素只包含source pad, sink元素只包含sink pad，而filter元素同时包含这两者。
 
 ![在这里插入图片描述](https://img-blog.csdnimg.cn/18341cf90bca4b6d88b37135a7047cdf.png)
 ![在这里插入图片描述](https://img-blog.csdnimg.cn/ffa6793391c241798b97704174b663d2.png)
 ![在这里插入图片描述](https://img-blog.csdnimg.cn/252bf14cf30a46ba93515491cc5a5faf.png)一个demuxer包含一个sink pad，通过sink pad，muxed数据到达，然后有多个source pads，每个流都可以在容器（demuxer）中找到。
![在这里插入图片描述](https://img-blog.csdnimg.cn/77d5363e0d754c93a014260c5e4ab77f.png)
为了完整起见，这里有一个简化的管道，其中包含一个demuxer和两个分支，一个用于音频，一个用于视频。下面的示例只建造音频部分。

![在这里插入图片描述](https://img-blog.csdnimg.cn/5ceccdf808474d4c85d0be49bd63cf02.png)用demuxer处理数据时，最复杂的是在它们接收到一些数据并有机会查看容器以了解里面是什么之前，它们不能产生任何信息。也就是说，demuxer开始时没有其他元素可以链接的source pad，因此管道必须在它们处终止。

解决方案是构建从source到demuxer的管道，并将其设置为run (play)。当demuxer收到足够的信息来知道容器中流的数量和类型时，它将开始创建source pads。这是我们完成管道构建并将其连接到新添加的demuxer pads的正确时机。

简单起见，在本例中，我们将只链接到audio pad，而忽略视频。

## 动态Hello world
将这些代码复制到一个名为basic-tutorial-3.c的文本文件中(或者在GStreamer安装中找到它)。

```c
/* filename: basic-tutorial-3.c */
#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it to callbacks */
/* 这里存下了所有需要的局部变量，因为本教程中会有回调函数，使用struct比较方便 */
typedef struct _CustomData
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *convert;
  GstElement *resample;
  GstElement *sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement * src, GstPad * pad,
    CustomData * data);

int
main (int argc, char *argv[])
{
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  /* 我的理解是：uridecodebin创建的时候，没有初始化内部source pad，是因为内部带有demuxer */
  data.source = gst_element_factory_make ("uridecodebin", "source");
  data.convert = gst_element_factory_make ("audioconvert", "convert");
  data.resample = gst_element_factory_make ("audioresample", "resample");
  data.sink = gst_element_factory_make ("autoaudiosink", "sink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");

  if (!data.pipeline || !data.source || !data.convert || !data.resample
      || !data.sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.convert,
      data.resample, data.sink, NULL);
  
  /* 这儿没有吧source连接上，因为这个时候还没有source pad */
  if (!gst_element_link_many (data.convert, data.resample, data.sink, NULL)) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Set the URI to play */
  g_object_set (data.source, "uri",
      "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Connect to the pad-added signal */
  /* 在这里把回调函数的src data变量指定参数*/
  g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler),
      &data);
  
  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
  
  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

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
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state,
                &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state),
                gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}

/* This function will be called by the pad-added signal */
static void
pad_added_handler (GstElement * src, GstPad * new_pad, CustomData * data)
{
  GstPad *sink_pad = gst_element_get_static_pad (data->convert, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  g_print("1\n");
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad),
      GST_ELEMENT_NAME (src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (sink_pad)) {
    g_print ("We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("It has type '%s' which is not raw audio. Ignoring.\n",
        new_pad_type);
    goto exit;
  }

  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref (sink_pad);
}

```
## 概述

```c
/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *convert;
  GstElement *resample;
  GstElement *sink;
} CustomData;

```
到目前为止，我们已经将所需的所有信息(指向GstElements的指针)保存为全局变量。由于本教程(以及大多数实际应用程序)涉及回调，我们将把所有数据分组到一个结构中，以便于处理。
```c
/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);
```
这是一个回调函数的声明，之后我们会使用到。
```c
/* Create the elements */
data.source = gst_element_factory_make ("uridecodebin", "source");
data.convert = gst_element_factory_make ("audioconvert", "convert");
data.resample = gst_element_factory_make ("audioresample", "resample");
data.sink = gst_element_factory_make ("autoaudiosink", "sink");
```

我们像往常一样创建element。uridecodebin将在内部实例化所有必要的元素(sources、demuxers和decoders)，以将URI转换为原始音频（raw audio）和/或视频流（raw video）。它做的工作只有playbin（第一节直接建立的管道）的一半。由于它包含了demuxer，因为它的source pads最初不可获得，所以我们需要动态链接到它们。

audioconvert用于在不同的音频格式之间进行转换，确保这个示例在任何平台上都能工作，因为音频decoder生成的格式可能与音频sink期望的格式不同。

audioresample用于在不同的音频采样率之间进行转换，类似地，它可以确保这个示例在任何平台上都能工作，因为音频decoder产生的音频采样率可能不是音频sink支持的。

autoaudiosink与上一教程中提到的autovideosink在音频方面是等价的。它将把音频流渲染到声卡上。

```c
if (!gst_element_link_many (data.convert, data.resample, data.sink, NULL)) {
  g_printerr ("Elements could not be linked.\n");
  gst_object_unref (data.pipeline);
  return -1;
}
这里我们连接了elements converter，resample和sink，但我们没有将它们与source起来，因为在这一点上它不包含source pads。我们只是让这个分支(source pads和converter sink pad)保持不链接，直到后面才去处理。
```

```c
/* Set the URI to play */
g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
```
我们通过属性将文件的URI设置，就像我们在之前的教程中所做的那样。

## 信号

```c
/* Connect to the pad-added signal */
g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);

```
GSignals是GStreamer中的至关重要的。它们允许您在发生有趣的事情时收到通知(通过回调)。信号由一个唯一的名称标识，每个GObject对象都有自己的信号。

在这一行中，我们将添加source（uridecodebin元素）的“pad-added”信号。为此，我们使用g_signal_connect()并提供要使用的参数：回调函数(pad_addded_handler)和一个数据指针。GStreamer对这个数据指针不做任何事情，它只是将它转发给回调，以便我们可以与它共享信息。在本例中，我们将一个指针传递给专门为此目的构建的CustomData结构体。

GstElement生成的信号可以在其文档中找到，也可以使用基础教程10:GStreamer工具中描述的gst-inspect-1.0工具。

通过gst-inspect-1.0工具可以查到uridecodebin的Pad Templates信息和Signals信息。Pad的存在状态有Always、On request、Sometimes（接下来的课程中会介绍）。
![请添加图片描述](https://img-blog.csdnimg.cn/12d33083e5694d32bccbfef50296f0a9.png)
![请添加图片描述](https://img-blog.csdnimg.cn/3ebbc8ea98b0436d89345b1c85b589cc.png)


我们现在准备好了!只需将管道设置为PLAYING状态，并开始侦听总线中有趣的消息(如ERROR或EOS)，就像在前面的教程中一样。

## 回调函数
当我们的source element终于有足够的信息开始产生数据时，它将创建source pads，并触发“pad-added”信号。此时，我们的回调函数将被调用:

```c
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data);
```
 - src是触发信号的GstElement对象。在这个例子中，它只能是uridecodebin，因为它是我们唯一一个拥有该信号的element。信号处理程序的第一个参数总是触发它的对象。

 - new_pad是刚刚添加到src元素的GstPad。这通常是我们想要连接的pad。

 - data是我们附加到信号时提供的指针。在这个例子中，我们使用它来传递CustomData指针。
 

```c
GstPad *sink_pad = gst_element_get_static_pad (data->convert, "sink");
```
我们从CustomData中提取converter element地址，然后使用gst_element_get_static_pad()取得它的sink pad。这是我们想要连接new_pad的pad。在前面的教程中，我们将元素连接到元素上，`并让GStreamer选择适当的pad`。现在我们将直接连接pads。

```c
/* If our converter is already linked, we have nothing to do here */
if (gst_pad_is_linked (sink_pad)) {
  g_print ("We are already linked. Ignoring.\n");
  goto exit;
}

```
uridecodebin可以创建多个可能看起来合适的pads，对于每个pads，都会调用这个回调函数。一旦我们已经连接了，这些代码将阻止我们尝试连接到一个新的pad。

```c
/* Check the new pad's type */
new_pad_caps = gst_pad_get_current_caps (new_pad, NULL);
new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
new_pad_type = gst_structure_get_name (new_pad_struct);
if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
  g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
  goto exit;
}

```
现在我们将检查这个新pad将要输出的数据类型，因为我们只对产生音频的pad感兴趣。我们之前已经创建了一个处理音频的管道(一个audioconvert连接到一个audioreample和一个autoaudiosink)，我们将不能将audioconvert连接到一个处理视频的pad。

gst_pad_get_current_caps()获取pad的当前功能(即当前输出的数据类型)，包装在一个GstCaps结构体中。可以使用gst_pad_query_caps()查询一个pad可以支持的所有可能的caps。一个pad可以提供许多功能（caps），因此GstCaps可以包含许多GstStructure，每个GstStructure代表不同的功能。pad上的当前caps将始终具有单个GstStructure并表示单个媒体格式，或者如果没有当前caps但将返回NULL。

因为，在本例中，我们知道我们想要的pad只有一种能力(音频)，所以我们使用gst_caps_get_structure()检索第一个GstStructure。

最后，通过gst_structure_get_name()，我们得到了结构体的名称，其中包含了格式的主要描述(实际上是它的媒体类型)。

如果名称不是audio/x-raw，这不是一个解码的audio pad，我们对它不感兴趣。如果是audio/x-raw，尝试链接:

```c
/* Attempt the link */
ret = gst_pad_link (new_pad, sink_pad);
if (GST_PAD_LINK_FAILED (ret)) {
  g_print ("Type is '%s' but link failed.\n", new_pad_type);
} else {
  g_print ("Link succeeded (type '%s').\n", new_pad_type);
}

```
gst_pad_link()尝试连接两个pad。与gst_element_link()的情况一样，必须指定从source到sink的链接，并且两个pad（拥有两个pad的元素）必须由位于同一个bin(或pipeline)。

我们完成了!当出现正确类型的pad时，它将链接到音频处理管道的其余部分，并继续执行，直到ERROR或EOS。然而，我们将通过引入状态的概念从本教程中介绍更多的内容。

## GStreamer States
我们已经讨论了一些状态，当我们说播放不会开始直到你把管道带到PLAYING状态。我们将在这里介绍其他状态及其含义。在GStreamer中有4种状态:
|State|Description|
|--|--|
| NULL | 元素的NULL状态或初始状态|
|READY|元素已经准备好进入到PAUSED|
|PAUSED|元素处于暂停状态，准备接受和处理数据。然而，sink元素只接受一个缓冲区buffer，然后阻塞。
|PLAYING|元素在PLAYING，时钟在运行，数据在流动 |

你只能在相邻的状态之间移动，也就是说，你不能从NULL到PLAYING，你必须经过中间的READY和PAUSED状态。如果你将管道设置为PLAYING, GStreamer会为你进行中间过渡操作。

```c
case GST_MESSAGE_STATE_CHANGED:
  /* We are only interested in state-changed messages from the pipeline */
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    g_print ("Pipeline state changed from %s to %s:\n",
        gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
  }
  break;

```
我们添加了这段代码，它监听关于状态变化的总线消息，并将它们打印在屏幕上，以帮助您理解转换。`每个元素`都会在总线上放置有关其当前状态的消息，因此我们将其过滤掉，只侦听来自管道的消息。

大多数应用程序只需要关心PLAYING开始播放，然后PAUSE执行暂停，然后在程序程序完后，设置到NULL，释放掉所有资源。

## 练习
一般上，动态pad链接对于许多程序员来说是一个困难的话题。通过实例化一个autovideosink(可能前面有一个videoconvert)并在demuxer的source pad出现时进行链接。提示:你已经在屏幕上打印了视频pad的类型。

你现在应该看到(并听到)与基础教程1中相同的电影:Hello world!在那个教程中，你使用了playbin，它是一个方便的元素，可以自动为你处理所有的demuxing和pad连接。大部分的播放教程都是关于playbin的。

（1）因为按照教程的提示管道无法正常运行（应该是pads没有协商成功），通过网上搜寻找个一个可以播放的命令行：

> gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux ! vp8dec ! videoconvert ! autovideosink

上面命令之前先输入调试环境变量

> export GST_DEBUG_DUMP_DOT_DIR=/home/lieryang/Desktop/gst-study/basic-tutorial/graphs

按上述命令行建立的管道应用程序可以播放视频，但是，音视频一起播放不能完成（`待解决`）

## 总结
在本教程中，你学到了:

 - 如何使用GSignals通知事件
 - 如何直接连接GstPads而不是它们的element。
 - GStreamer元素的各种状态。

还可以组合这些项来构建动态管道，它不是在程序开始时定义的，而是在有关媒体的信息可用时创建的。

您现在可以继续学习基本教程，并在基本教程4:Time management 中学习执行查找和与时间相关的查询，或者移动到播放教程，并获得关于playbin元素的更多见解。

请记住，在这个页面的附件中，您应该可以找到本教程的完整源代码和构建它所需的任何附件文件。很高兴在这里见到你，希望很快见到你!
## 1 补充
### 1.1 引用计数

```c
 /* 获取该元素有多少引用计数，引用计数数等于0时，
 * 该元素内存会被是否，但是没有赋值element = NULL
 */
GST_OBJECT_REFCOUNT_VALUE(element)
/* 调用gst_object_unref减少一个引用数，并且给 ata.pipeline = NULL */
gst_clear_object (&(element));
```

[参考：Play WebM streamed over HTTP using GStreamer’s souphttpsrc](https://tewarid.github.io/2011/10/11/play-webm-streamed-over-http-using-gstreamer%27s-souphttpsrc.html)

## 2 代码分析
&emsp;&emsp;主要分析一些官方手册这一节没有介绍到的函数。
### 2.1 通过element和pad名字检索到已存在的GstPad
```c
GstPad* gst_element_get_static_pad (GstElement *element, 
                                    const gchar *name);
```
&emsp;&emsp;这是通过 `gst-inspect-1.0 audioconvert` 命令查看Pad，由图片信息可知道，pad的名字是sink和src。所以通过该函数可以得到Pad。只能查找Availability：Always的元素的Pad。name等于sink或者src。
![PAD](https://img-blog.csdnimg.cn/7f9aa96496ea4a9888516679968d0753.png)
### 2.2 查看Pad类型

```c
/* 获得pad的Capabilities地址 */
GstCaps * gst_pad_get_current_caps (GstPad *pad);
/* 获得Capabilities中包含结构体信息 */
GstStructure * gst_caps_get_structure (const GstCaps *caps,
                                        guint index);
/* 读出type */
const gchar * gst_structure_get_name (const GstStructure *structure);


struct _GstStructure {
  GType type;

  /*< private >*/
  GQuark name;
};

```
### 2.4 Message和Cap都继承于mini_object
```c
struct _GstCaps {
  GstMiniObject mini_object;
};

struct _GstMessage
{
  GstMiniObject   mini_object;

  /*< public > *//* with COW */
  GstMessageType  type;
  guint64         timestamp;
  GstObject      *src;
  guint32         seqnum;

  /*< private >*//* with MESSAGE_LOCK */
  GMutex          lock;                 /* lock and cond for async delivery */
  GCond           cond;
};
```
&emsp;&emsp;释放资源函数


```c
static inline void
gst_caps_unref (GstCaps * caps)
{
  gst_mini_object_unref (GST_MINI_OBJECT_CAST (caps));
}

static inline void
gst_message_unref (GstMessage * msg)
{
  gst_mini_object_unref (GST_MINI_OBJECT_CAST (msg));
}
```


## 3 函数总结
```c
/*1.查看Message信息是那个element发出*/
GST_MESSAGE_SRC(message) 

/*2.从message读取管道状态*/
void gst_message_parse_state_changed (GstMessage *message, 
                                      GstState *oldstate,
                                      GstState *newstate, 
                                      GstState *pending);
/*3.获得状态字符串*/
const gchar* gst_element_state_get_name (GstState state);

/*4.*/
GstPad* gst_element_get_static_pad(GstElement *element, 
                                   const gchar *name);
```

## 4. 继承关系
### 4.1 GstObject
```c
GObject
    ╰──GInitiallyUnowned
        ╰──GstObject
            ╰──GstAllocator
            ╰──GstBufferPool
            ╰──GstBus
            ╰──GstClock
            ╰──GstControlBinding
            ╰──GstControlSource
            ╰──GstDevice
            ╰──GstDeviceMonitor
            ╰──GstDeviceProvider
            ╰──GstElement
            ╰──GstPad
            ╰──GstPadTemplate
            ╰──GstPlugin
            ╰──GstPluginFeature
            ╰──GstRegistry
            ╰──GstStream
            ╰──GstStreamCollection
            ╰──GstTask
            ╰──GstTaskPool
            ╰──GstTracer
            ╰──GstTracerRecord
```
### 4.2 GstMiniObject
&emsp;&emsp;GstMiniObject是一个种简单的，能够实现可计数类型结构。查看该结构并没有继承GObject对象，所以，任何继承于GstMiniObject的类都不可以使用GObject类的功能。主要要区别GstObject和GstMiniObject。GstObject中有属性、信号，后者没有。

```c
struct _GstMiniObject {
  GType   type;

  /*< public >*/ /* with COW */
  gint    refcount; 
  gint    lockstate;
  guint   flags;

  GstMiniObjectCopyFunction copy;
  GstMiniObjectDisposeFunction dispose;
  GstMiniObjectFreeFunction free;

  /* < private > */
  /* Used to keep track of weak ref notifies and qdata */
  guint n_qdata;
  gpointer qdata;
};
```

```c
GstMiniObject
    ╰──GstBuffer /*缓冲*/
    ╰──GstCaps /*Pad的相关信息*/
    ╰──GstMessage /*管道的消息*/
    ╰──GstEvent
    ╰──GstQuery
```

## 5. 获得name宏和函数总结
name属性是gstobject类中新增的
```c
#define GST_PAD_NAME(pad)		  (GST_OBJECT_NAME(pad))
#define GST_ELEMENT_NAME(elem)    (GST_OBJECT_NAME(elem))

#define gst_pad_get_name(pad)       gst_object_get_name(GST_OBJECT_CAST (pad))
#define gst_element_get_name(elem)  gst_object_get_name(GST_OBJECT_CAST(elem))
gchar*  gst_object_get_name		    (GstObject *object);
```
&emsp;&emsp;从上面不难发现，无论Pad还是Element得到对象名称的时候，宏定义后面使用的函数都是相同的。
