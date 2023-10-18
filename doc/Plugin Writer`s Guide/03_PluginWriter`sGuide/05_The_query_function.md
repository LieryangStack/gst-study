# 5 The query function

通过query函数，元素将收到查询，并必须响应查询。这些查询包括位置、持续时间等，还包括元素支持的格式和调度模式。查询可以在上游和下游传输，所以你可以在sink pad和source pad上接收它们。

下面是我们在元素的source pad上安装的一个非常简单的查询函数。

```c
static gboolean gst_my_filter_src_query (GstPad    *pad,
                                         GstObject *parent,
                                         GstQuery  *query);

[..]

static void
gst_my_filter_init (GstMyFilter * filter)
{
[..]
  /* configure event function on the pad before adding
   * the pad to the element */
  gst_pad_set_query_function (filter->srcpad,
      gst_my_filter_src_query);
[..]
}

static gboolean
gst_my_filter_src_query (GstPad    *pad,
                 GstObject *parent,
                 GstQuery  *query)
{
  gboolean ret;
  GstMyFilter *filter = GST_MY_FILTER (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
      /* we should report the current position */
      [...]
      break;
    case GST_QUERY_DURATION:
      /* we should report the duration here */
      [...]
      break;
    case GST_QUERY_CAPS:
      /* we should report the supported caps here */
      [...]
      break;
    default:
      /* just call the default handler */
      ret = gst_pad_query_default (pad, parent, query);
      break;
  }
  return ret;
}


```
对于未知的查询，最好调用默认的查询处理程序gst_pad_query_default()。根据查询类型，默认处理程序将转发查询或简单地取消引用它。