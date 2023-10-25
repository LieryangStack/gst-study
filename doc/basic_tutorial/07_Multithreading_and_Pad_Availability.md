## 目标
GStreamer自动处理多线程，但在某些情况下，您可能需要手动解耦线程。本教程展示了如何做到这一点，此外，还完成了关于Pad可用性的阐述。更准确地说，本文档解释了:

 - 如何为管道的某些部分创建新的执行线程
 - Pad的可用性是什么
 - 如何复制流
 

## 介绍
### 多线程
GStreamer是一个多线程框架。这意味着，在内部，它会根据需要创建和销毁线程，例如，从应用程序线程解耦流媒体。此外，插件也可以为自己的处理创建线程，例如，视频解码器可以创建4个线程，以充分利用4核CPU。

最重要的是，在构建管道时，应用程序可以明确指定分支(管道的一部分)运行在不同的线程上(例如，让音频和视频解码器同时执行)。

这是使用queue成员完成的，工作原理如下。sink pad只是将数据放入队列并返回控制权。在另一个线程上，数据被出队列并推送到下游。这个元素还用于缓冲，稍后在流处理教程中会看到。队列的大小可以通过属性来控制。

### The example pipeline
这个例子构建了下面的管道:
![在这里插入图片描述](https://img-blog.csdnimg.cn/5025e7f41ff74252b550272e28e74403.png)音源是一个合成的音频信号(连续的音调)，使用tee元素分割(它把所有接受到的sink pad内容发送到src pad)。然后，一个分支将信号发送到声卡，另一个渲染波形视频并将其发送到屏幕。

如图所示，`队列创建了一个新线程`，因此这个管道运行在3个线程中。具有多个sink的管道通常需要多线程，`因为要同步`，sink通常会阻塞执行，直到所有其他sink都准备好，而如果只有一个线程，被第一个sink阻塞，它们就无法准备好。

### Request pads
在基本教程3:动态管道中，我们看到一个元素(uridecodebin)一开始没有pad，当数据开始流动，元素了解媒体时，它们就出现了。这些被称为**Sometimes Pads**，与常规的Pads相比，这些常规Pad它们总是可用，被称为**Always Pads**。

第三种pad是**Request pad**，它是按需创建的。经典的例子是tee元素，它只有一个sink pad，没有初始source pad:需要请求它们，然后tee添加它们这些被请求的pad。通过这种方式，输入流可以被复制任意多次。缺点是链接具有请求填充的元素不像链接Always Pads的那样自动，这将在本示例的演练中说明。

`此外，要在播放PLAYING或暂停PAUSED状态下请求(或释放)Pad，您需要采取本教程中没有描述的额外注意事项(Pad blocking)。不过，请求(或释放)处于NULL或READY状态的pad是安全的。`

让我们看看代码。


## Simple multithreaded example
```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline, *audio_source, *tee, *audio_queue, *audio_convert, *audio_resample, *audio_sink;
  GstElement *video_queue, *visual, *video_convert, *video_sink;
  GstBus *bus;
  GstMessage *msg;
  GstPad *tee_audio_pad, *tee_video_pad;
  GstPad *queue_audio_pad, *queue_video_pad;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  audio_source = gst_element_factory_make ("audiotestsrc", "audio_source");
  tee = gst_element_factory_make ("tee", "tee");
  audio_queue = gst_element_factory_make ("queue", "audio_queue");
  audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
  audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
  audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
  video_queue = gst_element_factory_make ("queue", "video_queue");
  visual = gst_element_factory_make ("wavescope", "visual");
  video_convert = gst_element_factory_make ("videoconvert", "csp");
  video_sink = gst_element_factory_make ("autovideosink", "video_sink");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new ("test-pipeline");

  if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample || !audio_sink ||
      !video_queue || !visual || !video_convert || !video_sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Configure elements */
  g_object_set (audio_source, "freq", 215.0f, NULL);
  g_object_set (visual, "shader", 0, "style", 1, NULL);

  /* Link all elements that can be automatically linked because they have "Always" pads */
  gst_bin_add_many (GST_BIN (pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
      video_queue, visual, video_convert, video_sink, NULL);
  if (gst_element_link_many (audio_source, tee, NULL) != TRUE ||
      gst_element_link_many (audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
      gst_element_link_many (video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Manually link the Tee, which has "Request" pads */
  tee_audio_pad = gst_element_get_request_pad (tee, "src_%u");
  g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
  queue_audio_pad = gst_element_get_static_pad (audio_queue, "sink");
  tee_video_pad = gst_element_get_request_pad (tee, "src_%u");
  g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
  queue_video_pad = gst_element_get_static_pad (video_queue, "sink");
  if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
      gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Tee could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  gst_object_unref (queue_audio_pad);
  gst_object_unref (queue_video_pad);

  /* Start playing the pipeline */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad (tee, tee_audio_pad);
  gst_element_release_request_pad (tee, tee_video_pad);
  gst_object_unref (tee_audio_pad);
  gst_object_unref (tee_video_pad);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  return 0;
}

```

## 代码讲解
```c
/* Create the elements */
audio_source = gst_element_factory_make ("audiotestsrc", "audio_source");
tee = gst_element_factory_make ("tee", "tee");
audio_queue = gst_element_factory_make ("queue", "audio_queue");
audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
  audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
video_queue = gst_element_factory_make ("queue", "video_queue");
visual = gst_element_factory_make ("wavescope", "visual");
video_convert = gst_element_factory_make ("videoconvert", "video_convert");
video_sink = gst_element_factory_make ("autovideosink", "video_sink");

```
上图中的所有元素都在这里实例化:

audiotestsrc产生合成音调。wavescope消耗音频信号并渲染波形，就好像它是一个简易的示波器。我们已经使用过autoaudiosink和autovideosink。

转换元素(audioconvert、audioresample和videoconvert)是保证管道可以链接的必要元素。实际上，音频和视频sink Capabilities取决于硬件，在设计时您不知道它们是否与audiotestsrc和wavescope产生的Caps匹配。但是，如果Caps匹配，这些元件将以“pass-through”模式工作，不会修改信号，对性能的影响可以忽略不计。

```c
/* Configure elements */
g_object_set (audio_source, "freq", 215.0f, NULL);
g_object_set (visual, "shader", 0, "style", 1, NULL);
```

为了更好地演示，做了一些小调整:audiotestsrc的“freq”属性控制了波的频率(215Hz使波在窗口中看起来几乎是静止的)，这种样式和wavescope的着色器使波是连续的。使用基本教程10:GStreamer工具中描述的gst-inspect-1.0工具来学习这些元素的所有属性。

```c
/* Link all elements that can be automatically linked because they have "Always" pads */
gst_bin_add_many (GST_BIN (pipeline), audio_source, tee, audio_queue, audio_convert, audio_sink,
    video_queue, visual, video_convert, video_sink, NULL);
if (gst_element_link_many (audio_source, tee, NULL) != TRUE ||
    gst_element_link_many (audio_queue, audio_convert, audio_sink, NULL) != TRUE ||
    gst_element_link_many (video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
  g_printerr ("Elements could not be linked.\n");
  gst_object_unref (pipeline);
  return -1;
}

```
这段代码将所有元素添加到管道中，然后链接可以自动链接的元素(如注释所示，总是Always Pads的元素)。

gst_element_link_many()实际上可以将元素（Request Pads的元素）起来。它在内部请求pads，因此您不必担心链接的元素Always或Request Pads。这看起来可能很奇怪，但实际上并不方便，因为用户仍然需要在之后释放请求的Pad，而且如果gst_element_link_many()自动请求Pad，那么很容易忘记。要避免麻烦，一定要手动请求请求Pads，如下面的代码块所示。

```c
/* Manually link the Tee, which has "Request" pads */
tee_audio_pad = gst_element_get_request_pad (tee, "src_%u");
g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
queue_audio_pad = gst_element_get_static_pad (audio_queue, "sink");
tee_video_pad = gst_element_get_request_pad (tee, "src_%u");
g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
queue_video_pad = gst_element_get_static_pad (video_queue, "sink");
if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
    gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
  g_printerr ("Tee could not be linked.\n");
  gst_object_unref (pipeline);
  return -1;
}
gst_object_unref (queue_audio_pad);
gst_object_unref (queue_video_pad);

```
要链接Request Pads，需要通过向元素“requesting”它们来获得。一个元素可能能够生成不同类型的Request Pads，因此，在请求它们时，必须提供所需的Pad Template名称。在tee元素的文档中，我们看到它有两个pad templates，分别名为“sink”(表示它的sink pad)和“src_%u”(表示请求pad)。我们使用gst_element_get_request_pad()从tee请求两个pad(音频和视频分支)。

然后，我们从下游元素获取Pads，上面提及到的Request Pads会去连接这些下游元素的Pads。这些下游元素都是普通的Always Pads，因此我们使用gst_element_get_static_pad()获取它们。

最后，我们使用gst_pad_link()链接pad。这是gst_element_link()和gst_element_link_many()内部使用的函数。

我们获得的sink pad需要用gst_object_unref()释放。在程序结束时，当我们不再需要它们时，请求Pads将被释放。

然后我们将管道设置为正常运行，并等待直到产生错误信息或EOS。剩下唯一要做的就是清理请求的pad:

```c
/* Release the request pads from the Tee, and unref them */
gst_element_release_request_pad (tee, tee_audio_pad);
gst_element_release_request_pad (tee, tee_video_pad);
gst_object_unref (tee_audio_pad);
gst_object_unref (tee_video_pad);

```
gst_element_release_request_pad()将pad从tee中释放，但仍然需要使用gst_object_unref()解除引用(释放)。

## 总结
本教程展示了:

 - 如何使用队列元素使管道的各个部分在不同的线程上运行。
 - 什么是Request Pad，以及如何使用gst_element_get_request_pad()、gst_pad_link()和gst_element_release_request_pad()将元素与Request Pad链接起来。
 - 如何使用tee元素使相同的流在不同的分支中可用。

下一个教程建立在这个教程的基础上，展示如何手动将数据注入和从运行的管道中提取。

很高兴在这里见到你，希望很快见到你!