# 11 Tagging (Metadata and Streaminfo)

## 1 Overview

标签是存储在流中的信息片段，它们不是内容本身，而是对内容的描述。大多数媒体容器格式都以一种或另一种方式支持标记。Ogg 使用 VorbisComment，MP3 使用 ID3，AVI 和 WAV 使用 RIFF 的 INFO 列表块等。GStreamer为元素提供了一种通用的方法，以从流中读取标签并向用户公开这些标签。这些标签（至少是元数据）将成为管道内部流的一部分。这意味着从一种格式转码到另一种格式的文件将自动保留标签，只要输入和输出格式元素都支持标记。

在GStreamer中，标签分为两个类别，尽管应用程序不会注意到这一点。第一个类别称为元数据，第二个类别称为streaminfo。元数据是描述流内容的非技术部分的标签。它们可以更改，而无需完全重新编码流。示例包括“作者”、“标题”或“专辑”。不过，容器格式可能仍然需要被重写以适应标签。另一方面，streaminfo 是描述流内容技术方面的标签。要更改它们，需要重新编码流。示例包括“编解码器”或“比特率”。请注意，某些容器格式（如 ID3）将各种 streaminfo 标签存储为文件容器中的元数据，这意味着它们可以被更改，以便不再与文件中的内容匹配。尽管这使它们无效，但它们仍然被称为元数据，因为从技术上来说，它们可以在不重新编码整个流的情况下更改。带有此类元数据标签的文件将同时具有相同的标签：一次作为元数据，一次作为streaminfo。

在GStreamer中，没有专门的名称来表示读取标签的元素。有一些专门的元素（例如 id3demux），除了进行标签读取之外什么也不做，但任何GStreamer元素都可以在处理数据时提取标签，而大多数解码器、分解器和解析器都能够执行此操作。

标签写入器称为TagSetter。支持这两种功能的元素可用于标签编辑器，以进行快速标签更改（注意：在撰写本文时，原地标签编辑仍然受到较少的支持，通常需要提取/删除标签并使用新标签重新混合流）。

## 2 Reading Tags from Streams

标签的基本对象是GstTagList。从流中读取标签的元素应创建一个空的标签列表，并将其填充为单独的标签。可以使用gst_tag_list_new()创建空的标签列表。然后，元素可以使用gst_tag_list_add()或gst_tag_list_add_values()来填充列表。请注意，元素通常将元数据读取为字符串，但标签列表中的值不一定是字符串 - 它们必须是标签注册时的类型（每个预定义标签的API文档应包含类型信息）。请务必使用gst_value_transform()等函数，以确保数据具有正确的类型。在读取数据后，您可以使用TAG事件将标签发送到下游。当TAG事件到达接收端时，它将在应用程序获取的管道的GstBus上发布TAG消息。

在使用标签之前，我们当前要求核心了解标签的GType，因此必须首先注册所有标签。您可以使用gst_tag_register()将新标签添加到已知标签列表中。如果您认为该标签将在不仅仅在您自己的元素中有用的情况下，最好将其添加到gsttag.c中。这取决于您的决定。如果要在自己的元素中执行此操作，最简单的方法是在类初始化函数之一中注册标签，最好是_class_init()函数。


```c
static void
gst_my_filter_class_init (GstMyFilterClass *klass)
{
[..]
  gst_tag_register ("my_tag_name", GST_TAG_FLAG_META,
            G_TYPE_STRING,
            _("my own tag"),
            _("a tag that is specific to my own element"),
            NULL);
[..]
}
```

## 3 Writing Tags to Streams

标签写入器是标签读取器的相反。标签写入器仅考虑元数据标签，因为这是唯一必须写入流的标签类型。标签写入器可以通过三种方式接收标签：内部、应用程序和管道。内部标签是元素自己读取的标签，这意味着在这种情况下，标签写入器也是一个标签读取器。应用程序标签是通过TagSetter接口提供给元素的标签（这只是一个层）。管道标签是从管道内部提供给元素的标签。元素通过GST_EVENT_TAG事件接收这些标签，这意味着标签写入器应该实现事件处理程序。标签写入器负责将这三种标签合并为一个列表并将它们写入输出流。

下面的示例将从应用程序和管道接收标签，将它们合并并写入输出流。它实现了标签设置器，以便应用程序可以设置标签，并从传入的事件中检索管道标签。

请注意，此示例已过时，不再适用于GStreamer的1.0版本。

```c
GType
gst_my_filter_get_type (void)
{
[..]
    static const GInterfaceInfo tag_setter_info = {
      NULL,
      NULL,
      NULL
    };
[..]
    g_type_add_interface_static (my_filter_type,
                 GST_TYPE_TAG_SETTER,
                 &tag_setter_info);
[..]
}

static void
gst_my_filter_init (GstMyFilter *filter)
{
[..]
}

/*
 * Write one tag.
 */

static void
gst_my_filter_write_tag (const GstTagList *taglist,
             const gchar      *tagname,
             gpointer          data)
{
  GstMyFilter *filter = GST_MY_FILTER (data);
  GstBuffer *buffer;
  guint num_values = gst_tag_list_get_tag_size (list, tag_name), n;
  const GValue *from;
  GValue to = { 0 };

  g_value_init (&to, G_TYPE_STRING);

  for (n = 0; n < num_values; n++) {
    guint8 * data;
    gsize size;

    from = gst_tag_list_get_value_index (taglist, tagname, n);
    g_value_transform (from, &to);

    data = g_strdup_printf ("%s:%s", tagname,
        g_value_get_string (&to));
    size = strlen (data);

    buf = gst_buffer_new_wrapped (data, size);
    gst_pad_push (filter->srcpad, buf);
  }

  g_value_unset (&to);
}

static void
gst_my_filter_task_func (GstElement *element)
{
  GstMyFilter *filter = GST_MY_FILTER (element);
  GstTagSetter *tagsetter = GST_TAG_SETTER (element);
  GstData *data;
  GstEvent *event;
  gboolean eos = FALSE;
  GstTagList *taglist = gst_tag_list_new ();

  while (!eos) {
    data = gst_pad_pull (filter->sinkpad);

    /* We're not very much interested in data right now */
    if (GST_IS_BUFFER (data))
      gst_buffer_unref (GST_BUFFER (data));
    event = GST_EVENT (data);

    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_TAG:
        gst_tag_list_insert (taglist, gst_event_tag_get_list (event),
                 GST_TAG_MERGE_PREPEND);
        gst_event_unref (event);
        break;
      case GST_EVENT_EOS:
        eos = TRUE;
        gst_event_unref (event);
        break;
      default:
        gst_pad_event_default (filter->sinkpad, event);
        break;
    }
  }

  /* merge tags with the ones retrieved from the application */
  if ((gst_tag_setter_get_tag_list (tagsetter)) {
    gst_tag_list_insert (taglist,
             gst_tag_setter_get_tag_list (tagsetter),
             gst_tag_setter_get_tag_merge_mode (tagsetter));
  }

  /* write tags */
  gst_tag_list_foreach (taglist, gst_my_filter_write_tag, filter);

  /* signal EOS */
  gst_pad_push (filter->srcpad, gst_event_new (GST_EVENT_EOS));
}
```

请注意，通常情况下，元素在处理标签之前不会读取整个流。相反，它们会从每个sinkpad读取，直到它们收到数据（因为标签通常在第一个数据缓冲区之前到达），然后进行处理。