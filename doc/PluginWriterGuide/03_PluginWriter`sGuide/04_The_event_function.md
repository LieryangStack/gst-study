# 4 The event function

event函数会通知你datastream中发生的特殊事件(例如caps、end-of-stream、newsegment、tags等)。事件可以upstream和downstream传递，所以你可以通过sink pad和source pad接收它们。

下面是一个非常简单的事件函数，我们将其安装在元素的sink pads上。

```c
static gboolean gst_my_filter_sink_event (GstPad    *pad,
                                          GstObject *parent,
                                          GstEvent  *event);

[..]

static void
gst_my_filter_init (GstMyFilter * filter)
{
[..]
  /* configure event function on the pad before adding
   * the pad to the element */
  gst_pad_set_event_function (filter->sinkpad,
      gst_my_filter_sink_event);
[..]
}

static gboolean
gst_my_filter_sink_event (GstPad    *pad,
                  GstObject *parent,
                  GstEvent  *event)
{
  gboolean ret;
  GstMyFilter *filter = GST_MY_FILTER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      /* we should handle the format here */

      /* push the event downstream */
      ret = gst_pad_push_event (filter->srcpad, event);
      break;
    case GST_EVENT_EOS:
      /* end-of-stream, we should close down all stream leftovers here */
      gst_my_filter_stop_processing (filter);

      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      /* just call the default handler */
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

```
对于未知事件，最好调用默认的事件处理程序gst_pad_event_default()。根据事件类型，默认处理程序将转发事件或简单地取消引用它。默认情况下，CAPS事件是不转发的，因此我们需要在事件处理程序中执行此操作。

## 参考
[翻译自：The event function](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/eventfn.html?gi-language=c)