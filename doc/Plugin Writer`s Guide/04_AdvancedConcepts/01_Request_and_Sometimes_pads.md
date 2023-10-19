# 1 Request and Sometimes pads

到目前为止，我们只处理了总是可用的pad。然而，也有一些pad仅在某些情况下创建，或者仅在应用程序请求pad时创建。第一个有时被称为a;第二个被称为请求pad。pad的可用性(always, sometimes or request)可以在pad的模板中看到。本章将讨论它们各自在什么时候有用，如何创建它们，以及何时处理它们。
## Sometimes pads
“有时”pad是在特定条件下创建的pad，但不是在所有情况下创建的pad。这在很大程度上取决于流的内容:demuxers通常会解析流的首部，决定哪些基本流(视频、音频、字幕等)嵌入到系统流中，然后为每个基本流创建一个pad。根据自己的选择，它还可以为每个元素实例创建这些元素的多个实例。唯一的限制是每个新创建的pad都应该有一个唯一的名称。有时，当流数据被销毁时(即从暂停状态切换到就绪状态时)，补码也会被销毁。`你不应该在EOS上处理pad，因为有人可能会重新激活管道并寻求回到流结束点之前。在EOS之后流应该仍然有效，至少在流数据被处理之前。在任何情况下，元素都是该pad的所有者`。

下面的示例代码将解析一个文本文件，其中第一行是一个数字(n)。接下来的行都以数字(0到n-1)开始，这是应该发送数据的源pad的编号。
>3
0: foo
1: bar
0: boo
2: bye

解析此文件并创建动态“sometimes”pads的代码如下所示:

```c

typedef struct _GstMyFilter {
[..]
  gboolean firstrun;
  GList *srcpadlist;
} GstMyFilter;

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE (
  "src_%u",
  GST_PAD_SRC,
  GST_PAD_SOMETIMES,
  GST_STATIC_CAPS ("ANY")
);

static void
gst_my_filter_class_init (GstMyFilterClass *klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
[..]
  gst_element_class_add_pad_template (element_class,
    gst_static_pad_template_get (&src_factory));
[..]
}

static void
gst_my_filter_init (GstMyFilter *filter)
{
[..]
  filter->firstrun = TRUE;
  filter->srcpadlist = NULL;
}

/*
 * Get one line of data - without newline.
 */

static GstBuffer *
gst_my_filter_getline (GstMyFilter *filter)
{
  guint8 *data;
  gint n, num;

  /* max. line length is 512 characters - for safety */
  for (n = 0; n < 512; n++) {
    num = gst_bytestream_peek_bytes (filter->bs, &data, n + 1);
    if (num != n + 1)
      return NULL;

    /* newline? */
    if (data[n] == '\n') {
      GstBuffer *buf = gst_buffer_new_allocate (NULL, n + 1, NULL);

      gst_bytestream_peek_bytes (filter->bs, &data, n);
      gst_buffer_fill (buf, 0, data, n);
      gst_buffer_memset (buf, n, '\0', 1);
      gst_bytestream_flush_fast (filter->bs, n + 1);

      return buf;
    }
  }
}

static void
gst_my_filter_loopfunc (GstElement *element)
{
  GstMyFilter *filter = GST_MY_FILTER (element);
  GstBuffer *buf;
  GstPad *pad;
  GstMapInfo map;
  gint num, n;

  /* parse header */
  if (filter->firstrun) {
    gchar *padname;
    guint8 id;

    if (!(buf = gst_my_filter_getline (filter))) {
      gst_element_error (element, STREAM, READ, (NULL),
             ("Stream contains no header"));
      return;
    }
    gst_buffer_extract (buf, 0, &id, 1);
    num = atoi (id);
    gst_buffer_unref (buf);

    /* for each of the streams, create a pad */
    for (n = 0; n < num; n++) {
      padname = g_strdup_printf ("src_%u", n);
      pad = gst_pad_new_from_static_template (src_factory, padname);
      g_free (padname);

      /* here, you would set _event () and _query () functions */

      /* need to activate the pad before adding */
      gst_pad_set_active (pad, TRUE);

      gst_element_add_pad (element, pad);
      filter->srcpadlist = g_list_append (filter->srcpadlist, pad);
    }
  }

  /* and now, simply parse each line and push over */
  if (!(buf = gst_my_filter_getline (filter))) {
    GstEvent *event = gst_event_new (GST_EVENT_EOS);
    GList *padlist;

    for (padlist = srcpadlist;
         padlist != NULL; padlist = g_list_next (padlist)) {
      pad = GST_PAD (padlist->data);
      gst_pad_push_event (pad, gst_event_ref (event));
    }
    gst_event_unref (event);
    /* pause the task here */
    return;
  }

  /* parse stream number and go beyond the ':' in the data */
  gst_buffer_map (buf, &map, GST_MAP_READ);
  num = atoi (map.data[0]);
  if (num >= 0 && num < g_list_length (filter->srcpadlist)) {
    pad = GST_PAD (g_list_nth_data (filter->srcpadlist, num);

    /* magic buffer parsing foo */
    for (n = 0; map.data[n] != ':' &&
                map.data[n] != '\0'; n++) ;
    if (map.data[n] != '\0') {
      GstBuffer *sub;

      /* create region copy that starts right past the space. The reason
       * that we don't just forward the data pointer is because the
       * pointer is no longer the start of an allocated block of memory,
       * but just a pointer to a position somewhere in the middle of it.
       * That cannot be freed upon disposal, so we'd either crash or have
       * a memleak. Creating a region copy is a simple way to solve that. */
      sub = gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL,
          n + 1, map.size - n - 1);
      gst_pad_push (pad, sub);
    }
  }
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);
}



```
请注意，我们在每个地方都进行了大量检查，以确保文件中的内容是有效的。这样做有两个目的:首先，文件可能是错误的，在这种情况下，我们可以防止崩溃。第二个也是最重要的原因是——在极端情况下——该文件可能被恶意使用，导致插件中出现未定义的行为，这可能会导致安全问题。总是假定文件可能被用来做坏事。

## Request pads

“请求”填充与有时填充类似，不同之处在于，请求是根据元素外部的内容而不是元素内部的内容创建的。这个概念经常用于muxers，对于将要放置在输出系统流中的每个基本流，将要求一个sink pad。它还可以用于具有可变数量输入或输出填充的元素中，例如tee(多输出)或input-selector(多输入)元素。

要实现请求填充，需要提供一个带有GST_PAD_REQUEST的padtemplate，并在GstElement中实现request_new_pad虚拟方法。为了进行清理，你需要实现release_pad虚拟方法。

```c

static GstPad * gst_my_filter_request_new_pad   (GstElement     *element,
                         GstPadTemplate *templ,
                                                 const gchar    *name,
                                                 const GstCaps  *caps);

static void gst_my_filter_release_pad (GstElement *element,
                                       GstPad *pad);

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE (
  "sink_%u",
  GST_PAD_SINK,
  GST_PAD_REQUEST,
  GST_STATIC_CAPS ("ANY")
);

static void
gst_my_filter_class_init (GstMyFilterClass *klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
[..]
  gst_element_class_add_pad_template (klass,
    gst_static_pad_template_get (&sink_factory));
[..]
  element_class->request_new_pad = gst_my_filter_request_new_pad;
  element_class->release_pad = gst_my_filter_release_pad;
}

static GstPad *
gst_my_filter_request_new_pad (GstElement     *element,
                   GstPadTemplate *templ,
                   const gchar    *name,
                               const GstCaps  *caps)
{
  GstPad *pad;
  GstMyFilterInputContext *context;

  context = g_new0 (GstMyFilterInputContext, 1);
  pad = gst_pad_new_from_template (templ, name);
  gst_pad_set_element_private (pad, context);

  /* normally, you would set _chain () and _event () functions here */

  gst_element_add_pad (element, pad);

  return pad;
}

static void
gst_my_filter_release_pad (GstElement *element,
                           GstPad *pad)
{
  GstMyFilterInputContext *context;

  context = gst_pad_get_element_private (pad);
  g_free (context);

  gst_element_remove_pad (element, pad);
}

```
