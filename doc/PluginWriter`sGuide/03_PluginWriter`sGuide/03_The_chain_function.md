# 3 The chain function

链函数是所有数据处理都在其中进行的函数。在简单过滤器的情况下（本节示例的情况），_chain()函数大多是线性函数——因此对于每个传入的缓冲区，也将输出一个缓冲区。下面是一个非常简单的chain函数的实现:

```c

static GstFlowReturn gst_my_filter_chain (GstPad    *pad,
                                          GstObject *parent,
                                          GstBuffer *buf);

[..]

static void
gst_my_filter_init (GstMyFilter * filter)
{
[..]
  /* configure chain function on the pad before adding
   * the pad to the element */
  gst_pad_set_chain_function (filter->sinkpad,
      gst_my_filter_chain);
[..]
}

static GstFlowReturn
gst_my_filter_chain (GstPad    *pad,
                     GstObject *parent,
             GstBuffer *buf)
{
  GstMyFilter *filter = GST_MY_FILTER (parent);

  if (!filter->silent)
    g_print ("Have data of size %" G_GSIZE_FORMAT" bytes!\n",
        gst_buffer_get_size (buf));

  return gst_pad_push (filter->srcpad, buf);
}

```

显然，上面的代码没什么用。你通常会在那里处理数据，而不是打印出数据所在的位置。但请记住，缓冲区并不总是可写的。

在更高级的元素(进行事件处理的元素)中，你可能需要另外指定一个事件处理函数，在发送流事件时调用该函数(如caps、end-of-stream、newsegment、tags等)。

```c
static void
gst_my_filter_init (GstMyFilter * filter)
{
[..]
  gst_pad_set_event_function (filter->sinkpad,
      gst_my_filter_sink_event);
[..]
}



static gboolean
gst_my_filter_sink_event (GstPad    *pad,
                  GstObject *parent,
                  GstEvent  *event)
{
  GstMyFilter *filter = GST_MY_FILTER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      /* we should handle the format here */
      break;
    case GST_EVENT_EOS:
      /* end-of-stream, we should close down all stream leftovers here */
      gst_my_filter_stop_processing (filter);
      break;
    default:
      break;
  }

  return gst_pad_event_default (pad, parent, event);
}

static GstFlowReturn
gst_my_filter_chain (GstPad    *pad,
             GstObject *parent,
             GstBuffer *buf)
{
  GstMyFilter *filter = GST_MY_FILTER (parent);
  GstBuffer *outbuf;

  outbuf = gst_my_filter_process_data (filter, buf);
  gst_buffer_unref (buf);
  if (!outbuf) {
    /* something went wrong - signal an error */
    GST_ELEMENT_ERROR (GST_ELEMENT (filter), STREAM, FAILED, (NULL), (NULL));
    return GST_FLOW_ERROR;
  }

  return gst_pad_push (filter->srcpad, outbuf);
}

```

在某些情况下，元素还可以控制输入数据速率。在这种情况下，你可能想写一个所谓的基于循环的元素。源元素(只有源source pads)也可以是基于get-based的元素。这些概念将在本指南的高级部分和专门讨论源source pads的部分中进行解释。

## 参考
[翻译自：The chain function](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/chainfn.html?gi-language=c)