## 目标

用GStreamer构建的管道不需要完全关闭。数据就可以随时以各种方式注入到管道中，也可以从管道中提取出来。本教程展示了:
 - 如何将外部数据注入到通用GStreamer管道中。
 - 如何从通用的GStreamer管道中提取数据。
 - 如何访问和操作这些数据。

播放教程3:缩短管道解释如何在基于playbin的管道中实现相同的目标。

## 介绍

应用程序可以通过`多种方式`与GStreamer管道的数据流进行交互。本教程描述的是最简单的一个，因为它使用的是专门为这个目的而创建的元素。

用于向GStreamer管道中注入应用数据的元素是`appsrc`，用于将GStreamer数据提取回应用的对应元素是`appsink`。为了避免混淆名称，请从GStreamer的角度考虑:appsrc只是一个常规的source，它神奇地提供了从天而降的数据(实际上是由应用程序提供的)。appsink是一个普通的sink，流经GStreamer管道的数据在这里die(实际上由应用程序恢复)。

appsrc和appsink功能非常丰富，它们提供了自己的API(参见它们的文档)，可以通过链接到gstreamer-app库来访问这些API。然而，在本教程中，我们将使用一种更简单的方法，通过信号来控制它们。

appsrc可以在各种模式下工作:在`pull mode`模式下，它每次需要时都从应用程序请求数据。在`push mode`下，应用程序以自己的速度推送数据。此外，在push mode下，当已经提供了足够的数据时，应用程序可以选择在推送函数中阻塞block，也可以侦听enough-data和need-data的信号来控制流。这个例子实现了后一种方法。有关其他方法的信息可以在appsrc文档中找到。

## Buffers

数据以称为buffer的块的形式通过GStreamer管道传输。由于这个例子产生和消耗数据，我们需要了解GstBuffers。

source Pads产生buffers，buffers被Sink Pads消耗;GStreamer接收这些buffers并将它们在元素之间传递。

buffer只是表示一个数据单位，不要假设所有的缓冲区都具有相同的大小或表示相同的时间。你也不应该假设只有一个缓冲区进入元素，也就只有一个缓冲区出来。元素可以随意处理接收到的缓冲区。GstBuffers也可以包含多个实际的内存缓冲区。实际的内存缓冲区是使用GstMemory对象抽象出来的，一个GstBuffer可以包含多个GstMemory对象。

每个缓冲区都附加了`时间戳`和`持续时间`，它们描述了在什么时候缓冲区的内容应该被解码、渲染或显示。时间戳是一个非常复杂和微妙的主题，但现在这个简化的视图应该足够了。

例如，filesrc(一个读取文件的GStreamer元素)生成的buffer带有“ANY” Caps，没有时间戳信息。在分离demux之后(参见基本教程3:动态管道)buffer可以有一些特定的caps，例如“video/x-h264”。解码后，每个缓冲区将包含一个带有raw caps的单个视频帧(例如，“video/x-raw-yuv”)和非常精确的时间戳，表示该帧何时显示。

## This tutorial
本教程从两方面扩展了基本教程7:多线程和Pad可用性:首先，将audiotestsrc替换为将生成音频数据的appsrc。其次，将一个新的分支添加到tee中，这样数据将进入音频sink和波形显示也将复制到appsink中。appsink将信息上传回应用程序，然后应用程序只通知用户数据已收到，但它显然可以执行更复杂的任务。
![在这里插入图片描述](https://img-blog.csdnimg.cn/7a49736f28204047850586e5582cf140.png)


## 一种简易波形发生器
```c
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>

#define CHUNK_SIZE 1024   /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 44100 /* Samples per second we are sending */

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline, *app_source, *tee, *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
  GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
  GstElement *app_queue, *app_sink;

  guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
  gfloat a, b, c, d;     /* For waveform generation */

  guint sourceid;        /* To control the GSource */

  GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;

/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The ide handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
 * and is removed when appsrc has enough data (enough-data signal).
 */
static gboolean push_data (CustomData *data) {
  GstBuffer *buffer;
  GstFlowReturn ret;
  int i;
  GstMapInfo map;
  gint16 *raw;
  gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
  gfloat freq;

  /* Create a new empty buffer */
  buffer = gst_buffer_new_and_alloc (CHUNK_SIZE);

  /* Set its timestamp and duration */
  GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (data->num_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (CHUNK_SIZE, GST_SECOND, SAMPLE_RATE);

  /* Generate some psychodelic waveforms */
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  raw = (gint16 *)map.data;
  data->c += data->d;
  data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for (i = 0; i < num_samples; i++) {
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(500 * data->a);
  }
  gst_buffer_unmap (buffer, &map);
  data->num_samples += num_samples;

  /* Push the buffer into the appsrc */
  g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret);

  /* Free the buffer now that we are done with it */
  gst_buffer_unref (buffer);

  if (ret != GST_FLOW_OK) {
    /* We got some error, stop sending data */
    return FALSE;
  }

  return TRUE;
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void start_feed (GstElement *source, guint size, CustomData *data) {
  if (data->sourceid == 0) {
    g_print ("Start feeding\n");
    data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
  }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
static void stop_feed (GstElement *source, CustomData *data) {
  if (data->sourceid != 0) {
    g_print ("Stop feeding\n");
    g_source_remove (data->sourceid);
    data->sourceid = 0;
  }
}

/* The appsink has received a buffer */
static void new_sample (GstElement *sink, CustomData *data) {
  GstSample *sample;

  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  if (sample) {
    /* The only thing we do in this example is print a * to indicate a received buffer */
    g_print ("*");
    gst_buffer_unref (sample);
  }
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  g_main_loop_quit (data->main_loop);
}

int main(int argc, char *argv[]) {
  CustomData data;
  GstPad *tee_audio_pad, *tee_video_pad, *tee_app_pad;
  GstPad *queue_audio_pad, *queue_video_pad, *queue_app_pad;
  GstAudioInfo info;
  GstCaps *audio_caps;
  GstBus *bus;

  /* Initialize cumstom data structure */
  memset (&data, 0, sizeof (data));
  data.b = 1; /* For waveform generation */
  data.d = 1;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  data.app_source = gst_element_factory_make ("appsrc", "audio_source");
  data.tee = gst_element_factory_make ("tee", "tee");
  data.audio_queue = gst_element_factory_make ("queue", "audio_queue");
  data.audio_convert1 = gst_element_factory_make ("audioconvert", "audio_convert1");
  data.audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
  data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
  data.video_queue = gst_element_factory_make ("queue", "video_queue");
  data.audio_convert2 = gst_element_factory_make ("audioconvert", "audio_convert2");
  data.visual = gst_element_factory_make ("wavescope", "visual");
  data.video_convert = gst_element_factory_make ("videoconvert", "csp");
  data.video_sink = gst_element_factory_make ("autovideosink", "video_sink");
  data.app_queue = gst_element_factory_make ("queue", "app_queue");
  data.app_sink = gst_element_factory_make ("appsink", "app_sink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");

  if (!data.pipeline || !data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 ||
      !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual ||
      !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Configure wavescope */
  g_object_set (data.visual, "shader", 0, "style", 0, NULL);

  /* Configure appsrc */
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
  audio_caps = gst_audio_info_to_caps (&info);
  g_object_set (data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
  g_signal_connect (data.app_source, "need-data", G_CALLBACK (start_feed), &data);
  g_signal_connect (data.app_source, "enough-data", G_CALLBACK (stop_feed), &data);

  /* Configure appsink */
  g_object_set (data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
  g_signal_connect (data.app_sink, "new-sample", G_CALLBACK (new_sample), &data);
  gst_caps_unref (audio_caps);
  g_free (audio_caps_text);

  /* Link all elements that can be automatically linked because they have "Always" pads */
  gst_bin_add_many (GST_BIN (data.pipeline), data.app_source, data.tee, data.audio_queue, data.audio_convert1, data.audio_resample,
      data.audio_sink, data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, data.app_queue,
      data.app_sink, NULL);
  if (gst_element_link_many (data.app_source, data.tee, NULL) != TRUE ||
      gst_element_link_many (data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL) != TRUE ||
      gst_element_link_many (data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, NULL) != TRUE ||
      gst_element_link_many (data.app_queue, data.app_sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Manually link the Tee, which has "Request" pads */
  tee_audio_pad = gst_element_get_request_pad (data.tee, "src_%u");
  g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
  queue_audio_pad = gst_element_get_static_pad (data.audio_queue, "sink");
  tee_video_pad = gst_element_get_request_pad (data.tee, "src_%u");
  g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
  queue_video_pad = gst_element_get_static_pad (data.video_queue, "sink");
  tee_app_pad = gst_element_get_request_pad (data.tee, "src_%u");
  g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));
  queue_app_pad = gst_element_get_static_pad (data.app_queue, "sink");
  if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
      gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
      gst_pad_link (tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Tee could not be linked\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
  gst_object_unref (queue_audio_pad);
  gst_object_unref (queue_video_pad);
  gst_object_unref (queue_app_pad);

  /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
  bus = gst_element_get_bus (data.pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &data);
  gst_object_unref (bus);

  /* Start playing the pipeline */
  gst_element_set_state (data.pipeline, GST_STATE_PLAYING);

  /* Create a GLib Main Loop and set it to run */
  data.main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (data.main_loop);

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad (data.tee, tee_audio_pad);
  gst_element_release_request_pad (data.tee, tee_video_pad);
  gst_element_release_request_pad (data.tee, tee_app_pad);
  gst_object_unref (tee_audio_pad);
  gst_object_unref (tee_video_pad);
  gst_object_unref (tee_app_pad);

  /* Free resources */
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}

```
## 代码解释

创建管道的代码(第131 ~ 205行)是基本教程7:多线程和Pad可用性的扩展版本。它涉及到实例化所有元素，使用Always Pads来链接元素，以及手动链接tee元素的Request Pads。

关于appsrc和appsink元素的配置:

```c
/* Configure appsrc */
audio_caps_text = g_strdup_printf (AUDIO_CAPS, SAMPLE_RATE);
audio_caps = gst_caps_from_string (audio_caps_text);
g_object_set (data.app_source, "caps", audio_caps, NULL);
g_signal_connect (data.app_source, "need-data", G_CALLBACK (start_feed), &data);
g_signal_connect (data.app_source, "enough-data", G_CALLBACK (stop_feed), &data);

```
需要在appsrc上设置的第一个属性是caps。它指定了元素将要产生的数据类型，因此GStreamer可以检查是否可以与下游元素链接(也就是，下游元素是否能理解这种数据)。这个属性必须是一个GstCaps对象，使用gst_caps_from_string()可以很容易地从字符串构建这个对象。

然后，我们连接到`need-data`和`enough-data`信号。当appsrc的内部数据队列不足或几乎满时，它们分别由appsrc触发。我们将使用这些信号(分别)`启动`和`停止`我们的信号生成过程。

```c
/* Configure appsink */
g_object_set (data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
g_signal_connect (data.app_sink, "new-sample", G_CALLBACK (new_sample), &data);
gst_caps_unref (audio_caps);
g_free (audio_caps_text);

```

关于appsink配置，我们连接到new-sample信号，该信号在sink每次接收到buffer时发出。另外，信号发射需要通过emit-signals属性启用，因为默认情况下它是禁用的。

启动管道，等待消息和最后的清理工作照常进行。让我们回顾一下刚刚注册的回调函数:

```c
/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void start_feed (GstElement *source, guint size, CustomData *data) {
  if (data->sourceid == 0) {
    g_print ("Start feeding\n");
    data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
  }
}

```

当appsrc的内部队列即将耗尽(耗尽数据)时，将调用此函数。我们在这里要做的唯一一件事就是用g_idle_add()注册一个GLib idle函数，它将数据提供给appsrc，直到它再次填满。GLib的idle函数是GLib在主循环中调用的方法，只要它处于“空闲”状态，即没有更高优先级的任务需要执行时。显然，它需要一个GLib GMainLoop实例化并运行。

这只是appsrc允许的多种方法之一。特别是，缓冲区不需要使用GLib从主线程提供给appsrc，也不需要使用need-data和enough-data信号来与appsrc同步(尽管这据称是最方便的)。

我们注意到g_idle_add()返回的sourceid，通过sourceid因此稍后可以禁用它。

```c
/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
static void stop_feed (GstElement *source, CustomData *data) {
  if (data->sourceid != 0) {
    g_print ("Stop feeding\n");
    g_source_remove (data->sourceid);
    data->sourceid = 0;
  }
}

```
当appsrc的内部队列填满时，将调用此函数，因此我们停止推送push数据。这里我们只是使用g_source_remove()删除idle函数(idle函数实现为GSource)。

```c
/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The ide handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
 * and is removed when appsrc has enough data (enough-data signal).
 */
static gboolean push_data (CustomData *data) {
  GstBuffer *buffer;
  GstFlowReturn ret;
  int i;
  gint16 *raw;
  gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
  gfloat freq;

  /* Create a new empty buffer */
  buffer = gst_buffer_new_and_alloc (CHUNK_SIZE);

  /* Set its timestamp and duration */
  GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (data->num_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (CHUNK_SIZE, GST_SECOND, SAMPLE_RATE);

  /* Generate some psychodelic waveforms */
  raw = (gint16 *)GST_BUFFER_DATA (buffer);

```

这是给appsrc提供数据的函数。它有时会被GLib调用，并且速率超出我们的控制，但我们知道当它的工作完成时(当appsrc中的队列已满时)，我们将禁用它。

它的第一个任务是用gst_buffer_new_and_alloc()创建一个给定大小的新缓冲区(在本例中，它被任意设置为1024字节，也就是1KB)。

我们用CustomData.num_samples变量计算到目前为止生成的样本数量。因此，我们可以使用GstBuffer中的GST_BUFFER_TIMESTAMP宏为该buffer打时间戳。

由于我们生成的缓冲区大小相同，因此它们的持续时间是相同的，并使用GstBuffer中的GST_BUFFER_DURATION设置。

gst_util_uint64_scale()是一个实用函数，可以缩放(乘/除)可以很大的数字，而不用担心溢出。

用于缓冲区的字节可以在GstBuffer中通过GST_BUFFER_DATA访问(注意不要写入超过缓冲区的末尾:你已经分配了它，所以你知道它的大小)。

我们将跳过波形生成，因为它超出了本教程的范围(它只是一种有趣的生成漂亮迷幻波的方法)。
```c
/* Push the buffer into the appsrc */
g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret);

/* Free the buffer now that we are done with it */
gst_buffer_unref (buffer);

```
一旦我们准备好了缓冲区，我们就用push-buffer动作信号将它传递给appsrc(参见播放教程1末尾的信息框:Playbin用法)，然后gst_buffer_unref()它，因为我们不再需要它。

```c
/* The appsink has received a buffer */
static void new_sample (GstElement *sink, CustomData *data) {
  GstSample *sample;
  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  if (sample) {
    /* The only thing we do in this example is print a * to indicate a received buffer */
    g_print ("*");
    gst_sample_unref (sample);
  }
}

```
最后，这是appsink接收到缓冲区时调用的函数。我们使用拉采样动作信号来检索缓冲区，然后在屏幕上打印一些星号。我们可以使用GST_BUFFER_DATA宏获取数据指针，并使用GstBuffer中的GST_BUFFER_SIZE宏获取数据大小。请记住，这个缓冲区不必与我们在push_data函数中生成的缓冲区匹配，路径中的任何元素都可能以任何方式改变缓冲区(在这个例子中不是这样:appsrc和appsink之间的路径中只有一个tee，它不会改变缓冲区的内容)。

然后，我们使用gst_buffer_unref()函数调用缓冲区，完成本教程。

## 补充 gst_buffer_map和gst_buffer_unmap
### gst_buffer_map
```c
gboolean
gst_buffer_map (GstBuffer * buffer,
                GstMapInfo * info,
                GstMapFlags flags)
```
该函数用buffer中所有合并内存块的GstMapInfo填充info。

flags描述了对内存的期望访问。当标志是GST_MAP_WRITE时，buffer应该是可写的(从gst_buffer_is_writable返回)。

当buffer是可写的但内存是不可写的时，会自动创建并返回一个可写的副本。然后，`buffer memory的只读副本也会被这个可写副本取代`。

info中的内存在使用后应该用gst_buffer_unmap解除映射。

#### gst_buffer_unmap
```c
gst_buffer_unmap (GstBuffer * buffer,
                  GstMapInfo * info)
```
释放之前用gst_buffer_map映射的内存。
## 结论
本教程展示了应用程序如何做到:

 -  使用appsrcelement将数据注入管道。
 - 使用appsink元素从管道中检索数据。
 - 通过访问GstBuffer来操作这些数据。

在基于playbin的管道中，实现相同目标的方式略有不同。播放教程3:缩短管道展示如何做到这一点。

很高兴在这里见到你，希望很快见到你!
