## 目标
本教程展示了如何使用与时间相关的GStreamer工具。特别是:
 - 如何从管道中查询流的位置或持续时间等信息。
 - 如何在流中寻找(跳跃)到不同的位置(时间)。
 
## 介绍
GstQuery是一种机制，允许向元素或pad请求一条信息。在这个例子中，我们询问管道是否允许查找(有些sources，比如直播流，是不允许查找的seeking)。在允许的情况下，一旦电影已经运行了10秒，我们就会使用一个seek跳到不同的位置。

在之前的教程中，一旦我们设置好了管道并运行起来，我们的main函数就会等待接收一个错误或通过总线接收一个EOS。在这里，我们修改了这个函数，让它周期性地唤醒，并在pipeline中查询流的位置，以便将它打印在屏幕上。这类似于媒体播放器所做的，定期更新用户界面。

最后，查询流的持续时间，并在流发生变化时更新。

## seeking example
```c
/* filename: basic-tutorial-4.c */
/**
 * （1）GstQuery是向一个element或者pad询问一些信息的机制。
 * 在这个例子中我们会问pipeline是否支持跳转功能（实时流是不支持跳转功能的）
 * 如果支持跳转功能，那么在播放10s之后跳转到另一个位置。
 * （2）在前面的教程中，我们一旦建立pipeline并运行后，我们就是在等待ERROR或者EOS消息，
 * 这个例子中我们修改这部分，改成定时唤醒并查询pipeline当前播放的位置并在屏幕上显示出来。
*/
#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData
{
  GstElement *playbin;          /* Our one and only element */
  gboolean playing;             /* Are we in the PLAYING state? */
  gboolean terminate;           /* Should we terminate execution? */
  gboolean seek_enabled;        /* Is seeking enabled for this media? */
  gboolean seek_done;           /* Have we performed the seek already? */
  gint64 duration;              /* How long does this media last, in nanoseconds */
} CustomData;

/* Forward definition of the message processing function */
static void handle_message (CustomData * data, GstMessage * msg);

int
main (int argc, char *argv[])
{
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  data.playing = FALSE;
  data.terminate = FALSE;
  data.seek_enabled = FALSE;
  data.seek_done = FALSE;
  data.duration = GST_CLOCK_TIME_NONE;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  data.playbin = gst_element_factory_make ("playbin", "playbin");

  if (!data.playbin) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Set the URI to play */
  /* 每个Element都有属性可以通过
   * g_object_set()设置属性
   * g_object_get()读属性
   */
  g_object_set (data.playbin, "uri",
      "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  /* 流管开始 */
  /* NULL -> READY -> PAUSED -> PLAYING */
  ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.playbin);
    return -1;
  }

  /* Listen to the bus */
  /* 从流管上获得bus消息总线 */
  bus = gst_element_get_bus (data.playbin);
  do {
    /* 阻塞时间为 100 ms */
    msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
        GST_MESSAGE_DURATION);

    /* Parse message */
    if (msg != NULL) { /*msg有消息*/
      handle_message (&data, msg);
    } 
    else {
      /* We got no message, this means the timeout expired */
      if (data.playing) {
        gint64 current = -1;

        /* Query the current position of the stream */
        /* 查询流管当前时间，GST_FORMAT_TIME表示格式为时间格式 */
        if (!gst_element_query_position (data.playbin, GST_FORMAT_TIME,
                &current)) {
          g_printerr ("Could not query current position.\n");
        }

        /* If we didn't know it yet, query the stream duration */
        /* 查询流管总时间 */
        if (!GST_CLOCK_TIME_IS_VALID (data.duration)) {
          if (!gst_element_query_duration (data.playbin, GST_FORMAT_TIME,
                  &data.duration)) {
            g_printerr ("Could not query current duration.\n");
          }
        }

        /* Print current position and total duration */
        g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
            GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));

        /* If seeking is enabled, we have not done it yet, and the time is right, seek */
        if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
          g_print ("\nReached 10s, performing seek...\n");
          gst_element_seek_simple (data.playbin, GST_FORMAT_TIME,
              GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
          data.seek_done = TRUE;
        }
      }
    }
  } while (!data.terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);
  return 0;
}

static void
handle_message (CustomData * data, GstMessage * msg)
{
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
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nEnd-Of-Stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION:
      /* The duration has changed, mark the current one as invalid */
      data->duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED:{
      GstState old_state, new_state, pending_state;
      /* pending_state 即将发生的状态 */
      gst_message_parse_state_changed (msg, &old_state, &new_state,
          &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->playbin)) {
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
        /* Remember whether we are in the PLAYING state or not */
        data->playing = (new_state == GST_STATE_PLAYING);
        /* 如果处于播放状态
         * 查询流媒体开始和结束时间
         */
        if (data->playing) {
          /* We just moved to PLAYING. Check if seeking is possible */
          GstQuery *query;
          gint64 start, end;
          query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (data->playbin, query)) {
            gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start,
                &end);
            if (data->seek_enabled) {
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %"
                  GST_TIME_FORMAT "\n", GST_TIME_ARGS (start),
                  GST_TIME_ARGS (end));
            } else {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          } else {
            g_printerr ("Seeking query failed.");
          }
          gst_query_unref (query);
        }
      }
    }
      break;
    default:
      /* We should not reach here */
      g_printerr ("Unexpected message received.\n");
      break;
  }
  /*释放引用*/
  gst_message_unref (msg);
}

```
## 概述

```c
/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData {
  GstElement *playbin;  /* Our one and only element */
  gboolean playing;      /* Are we in the PLAYING state? */
  gboolean terminate;    /* Should we terminate execution? */
  gboolean seek_enabled; /* Is seeking enabled for this media? */
  gboolean seek_done;    /* Have we performed the seek already? */
  gint64 duration;       /* How long does this media last, in nanoseconds */
} CustomData;

/* Forward definition of the message processing function */
static void handle_message (CustomData *data, GstMessage *msg);

```
我们从定义一个包含所有信息的结构开始，这样我们就可以将它传递给其他函数。特别地，在这个例子中，我们将消息处理代码移到了单独的函数handle_message中，因为它变得有点太大了。

然后我们构建一个由一个元素组成的管道，一个playbin，我们已经在基础教程1:Hello world!中看到过。然而，playbin本身就是一个管道，在这种情况下，它是管道中唯一的元素，所以我们直接使用playbin元素。URI通过URI属性提供给playbin，管道被设置为播放状态。

```c
msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
    GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);
```

以前我们没有给gst_bus_timed_pop_filtered()提供超时时间，这意味着它在接收到消息之前不会返回。现在我们使用的超时时间是100毫秒，因此，如果在十分之一秒内没有收到消息，函数将返回NULL。我们将使用这个逻辑来更新我们的“UI”。

请注意，所需的超时时间必须指定为GstClockTime，单位为纳秒。表示不同时间单位的数字应该乘以GST_SECOND或GST_MSECOND等宏。这也使你的代码更具可读性。

如果我们得到一条消息，我们在handle_message函数(下一小节)中处理它，否则:

## 用户接口刷新

```c
/* We got no message, this means the timeout expired */
if (data.playing) { /* 视频流是否为播放状态 */

```
如果管道处于播放状态，就该刷新屏幕了。如果不在PLAYING状态，我们什么都不想做，因为大多数查询都会失败。

我们达到了大约每秒10次的刷新率，对于我们的UI来说已经足够了。我们将在屏幕上打印当前媒体位置，这可以通过查询管道来了解。这涉及到一些步骤，将在下一小节中介绍。但是，由于position和duration是常见的查询，GstElement提供了更简单、现成的替代方法:
 

```c
/* Query the current position of the stream */
if (!gst_element_query_position (data.pipeline, GST_FORMAT_TIME, &current)) {
  g_printerr ("Could not query current position.\n");
}

```

gst_element_query_position()隐藏了对query对象的管理，直接提供给我们结果。

```c
/* If we didn't know it yet, query the stream duration */
if (!GST_CLOCK_TIME_IS_VALID (data.duration)) {
  if (!gst_element_query_duration (data.pipeline, GST_FORMAT_TIME, &data.duration)) {
     g_printerr ("Could not query current duration.\n");
  }
}

```
现在是时候知道流的长度了，使用另一个GstElement函数:gst_element_query_duration()

```c
/* Print current position and total duration */
g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
    GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));

```
注意GST_TIME_FORMAT和GST_TIME_ARGS宏的用法，它们为用户提供了对GStreamer时间的友好表示。

```c
/* If seeking is enabled, we have not done it yet, and the time is right, seek */
if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
  g_print ("\nReached 10s, performing seek...\n");
  gst_element_seek_simple (data.pipeline, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
  data.seek_done = TRUE;
}

```
现在我们通过在pipeline上调用gst_element_seek_simple()来“简单”地执行查找。这种方法中隐藏了很多寻找的复杂性，这是一件好事!

让我们回顾一下参数:

 GST_FORMAT_TIME表示以时间单位指定目标。其他搜索格式使用不同的单位。

然后是GstSeekFlags，让我们回顾一下最常见的:

 - GST_SEEK_FLAG_FLUSH:在进行查找之前，丢弃当前管道中所有的数据。在管道重新填充和新数据开始出现时可能会暂停一段时间，但极大地提高了应用程序的“响应性”。如果没有提供此标志，“旧”的数据可能会显示一段时间，直到新位置出现在管道的末端。

 - GST_SEEK_FLAG_KEY_UNIT:对于大多数编码视频流，不可能查找任意位置，只能查找某些称为关键帧的帧。使用此标志时，寻道实际上会移动到最近的关键帧，并立即开始生成数据。如果未使用此标志，管道将在内部移动到最近的关键帧(没有其他选择)，但数据将不会显示，直到它到达请求的位置。最后一种选择更准确，但可能需要更长的时间。

 - GST_SEEK_FLAG_ACCURATE:一些媒体剪辑没有提供足够的索引信息，这意味着寻找任意位置是耗时的。在这些情况下，GStreamer通常会估计要查找的位置，并且通常工作得很好。如果这个精度对你的情况来说不够好(你看到寻求的不是你要求的精确时间)，那么提供这个标志。请注意，计算搜索位置可能需要更长的时间(在某些文件上非常长)。

最后，我们提供寻求的位置。由于我们请求的是GST_FORMAT_TIME，该值必须以纳秒为单位，因此为简单起见，我们以秒表示时间，然后乘以GST_SECOND。

## 消息 pump
handle_message函数处理通过管道总线接收到的所有消息。ERROR和EOS的处理与之前的教程相同，所以我们跳到有趣的部分:

```c
case GST_MESSAGE_DURATION:
  /* The duration has changed, mark the current one as invalid */
  data->duration = GST_CLOCK_TIME_NONE;
  break;

```
每当流的持续时间改变时，此消息就会被发送到总线上。在这里，我们只是将当前持续时间标记为无效，以便稍后重新查询。

```c
case GST_MESSAGE_STATE_CHANGED: {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
    g_print ("Pipeline state changed from %s to %s:\n",
        gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

    /* Remember whether we are in the PLAYING state or not */
    data->playing = (new_state == GST_STATE_PLAYING);

```
寻找和时间查询通常只在处于暂停或播放状态时得到有效的回复，因为所有元素都有机会接收信息并配置自己。这里，我们使用变量playing来跟踪管道是否处于playing状态。此外，如果我们刚刚进入播放状态，我们会执行第一个查询。我们询问管道当前流是否允许查找:

```c
if (data->playing) {
  /* We just moved to PLAYING. Check if seeking is possible */
  GstQuery *query;
  gint64 start, end;
  query = gst_query_new_seeking (GST_FORMAT_TIME);
  if (gst_element_query (data->pipeline, query)) {
    gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start, &end);
    if (data->seek_enabled) {
      g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
          GST_TIME_ARGS (start), GST_TIME_ARGS (end));
    } else {
      g_print ("Seeking is DISABLED for this stream.\n");
    }
  }
  else {
    g_printerr ("Seeking query failed.");
  }
  gst_query_unref (query);
}

```

gst_query_new_seeking()创建一个"seeking"类型的新查询对象，格式为GST_FORMAT_TIME。这表明我们通过指定想要移动到的新时间来进行搜索。我们还可以请求GST_FORMAT_BYTES，然后查找源文件中特定的字节位置，但这通常没什么用。

然后，使用gst_element_query()将这个query对象传递给管道。结果存储在同一个查询中，可以用gst_query_parse_seeking()轻松取得。它提取一个布尔值，表示是否允许搜索，以及可以搜索的范围。

当你使用完query对象时，不要忘记取消它。

就是这样!有了这些知识，就可以构建一个媒体播放器，它可以基于当前流的位置周期性地更新滑块，并允许通过移动滑块来查找!

## 结论
本教程展示了:

 - 如何使用GstQuery查询管道信息
 - 如何查看流是否支持seek，使用`gst_query_new_seeking(),gst_element_query(),gst_query_parse_seeking()`。

 - 如何使用`gst_element_query_position()`和`gst_element_query_duration()`获取位置和持续时间等公共信息

 - 如何使用`gst_element_seek_simple()`跳转到流中的任意位置

在这种状态下，所有这些操作都可以执行。

下一个教程将展示如何将GStreamer与图形用户界面工具包集成。

请记住，您应该可以在此页面上找到教程的完整源代码和构建教程所需的任何附件文件。

很高兴你能来这里，很快就能见到你!

[参考：Basic tutorial 4: Time management](https://gstreamer.freedesktop.org/documentation/tutorials/basic/time-management.html?gi-language=c)