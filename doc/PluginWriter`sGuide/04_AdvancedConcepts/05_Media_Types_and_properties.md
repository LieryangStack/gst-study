# 5 Media Types and Properties

有很多可能的媒体类型，用于在元素之间传递数据。实际上，每个新定义的元素都可以使用新的数据格式（尽管除非至少有另一个元素识别该格式，否则它将无用，因为没有任何东西可以与其链接）。

为了媒体类型能够有用，以及使自动插件系统等系统正常运行，有必要让所有元素就媒体类型的定义以及每种媒体类型所需的属性达成一致。GStreamer 框架本身只提供了定义媒体类型和参数的能力，但不固定媒体类型和参数的含义，也不强制新媒体类型的创建遵循某种标准。这是一个需要政策决定的事情，而不是技术系统来强制执行。

目前，政策很简单：

- 如果可以使用已经存在的媒体类型，不要创建新的媒体类型。

- 如果要创建新的媒体类型，首先与其他 GStreamer 开发人员讨论，可以通过 IRC、邮件列表等方式进行讨论。

- 尽量确保新格式的名称不会与已经存在的内容冲突，并且不要使用比实际需要更通用的名称。例如，"audio/compressed" 是一个太过通用的名称，不能很好地表示使用 mp3 编解码的音频数据。相反，"audio/mp3" 可能是一个合适的名称，或者 "audio/compressed" 可以存在，但要有一个属性来指示使用的压缩类型。

- 确保当你创建新的媒体类型时，要清晰地定义它，并将其添加到已知媒体类型的列表中，以便其他开发人员在编写自己的元素时能够正确使用这种媒体类型。

## 1 Building a Simple Format for Testing

如果你需要一种尚未在我们的已定义类型列表中定义的新格式，那么你可能需要一些有关媒体类型命名、属性等方面的一般准则。一个理想的媒体类型应当等同于由 IANA 定义的媒体类型，否则，它应采用 type/x-name 的形式，其中 type 表示这种媒体类型处理的数据类型（音频、视频等），而 name 应该是特定于这个特定类型的内容。音频和视频媒体类型应尽量支持通用的音频/视频属性（List of Defined Types），并可以使用自己的属性。要了解我们认为哪些属性有用的想法，可以再次查看列表。

花时间找到适合你的类型的属性集。没有必要着急。此外，进行实验通常是一个不错的主意。经验表明，理论上构思出来的类型通常是好的，但它们仍然需要实际使用来确保它们满足需求。确保你的属性名称不与其他类型中使用的类似属性发生冲突。如果它们匹配，确保它们具有相同的含义；不允许使用相同名称但具有不同类型的属性。

## 2 Typefind Functions and Autoplugging

仅仅定义数据类型是不够的。为了使随机数据文件被识别并播放，我们需要一种方式来从数据流中识别其类型。为此，引入了“类型识别”（Typefinding）。类型识别是一种检测数据流类型的过程。类型识别由两个独立的部分组成：首先，有无限数量的我们称之为类型识别函数的函数，它们每个都能从输入流中识别一个或多个类型。然后，其次，有一个小型引擎来注册和调用其中的每个函数。这就是类型识别核心。通常，在类型识别核心之上，你会编写一个自动插件（autoplugger），它能够使用这种类型检测系统来动态构建一个围绕输入流的管道。在这里，我们仅关注类型识别函数。

类型识别函数通常位于 gst-plugins-base/gst/typefind/gsttypefindfunctions.c 中，除非有很好的理由（比如库依赖性）将其放在其他地方。集中存放的原因是减少加载以侦测流类型所需的插件数量。以下是一个示例，它将识别以“RIFF”标签开头，然后是文件大小，然后是“AVI”标签的AVI文件：

```c
static GstStaticCaps avi_caps = GST_STATIC_CAPS ("video/x-msvideo");
#define AVI_CAPS gst_static_caps_get(&avi_caps)
static void
gst_avi_typefind_function (GstTypeFind *tf,
              gpointer     pointer)
{
  guint8 *data = gst_type_find_peek (tf, 0, 12);

  if (data &&
      GUINT32_FROM_LE (&((guint32 *) data)[0]) == GST_MAKE_FOURCC ('R','I','F','F') &&
      GUINT32_FROM_LE (&((guint32 *) data)[2]) == GST_MAKE_FOURCC ('A','V','I',' ')) {
    gst_type_find_suggest (tf, GST_TYPE_FIND_MAXIMUM, AVI_CAPS);
  }
}

GST_TYPE_FIND_REGISTER_DEFINE(avi, "video/x-msvideo", GST_RANK_PRIMARY,
    gst_avi_typefind_function, "avi", AVI_CAPS, NULL, NULL);

static gboolean
plugin_init (GstPlugin *plugin)
{
  return GST_TYPEFIND_REGISTER(avi, plugin);
}
```

请注意，gst-plugins/gst/typefind/gsttypefindfunctions.c 中有一些简化宏来减少代码量。如果你想提交带有新的类型识别函数的类型识别补丁，可以充分利用这些宏。

关于自动插件已在应用程序开发手册中详细讨论过。

## 3 List of Defined Types

以下是GStreamer中所有已定义的类型的列表。出于可读性的考虑，它们被分成不同的表格，分为音频、视频、容器、字幕和其他类型。在每个表格下面可能会有适用于该表格的一些注释。在定义每种类型时，我们尽量遵循 IANA 定义的类型和规则。

直接跳转到特定的表格：

- [Table of Audio Types](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#table-of-audio-types)

- [Table of Video Types](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#table-of-video-types)

- [Table of Container Types](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#table-of-container-types)

- [Table of Subtitle Types](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#table-of-subtitle-types)

- [Table of Other Types](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#table-of-other-types)

请注意，许多属性不是必需的，而是可选属性。这意味着大多数这些属性可以从容器头部提取，但是 - 如果容器头部未提供这些属性 - 也可以通过解析流头部或流内容来提取。政策是，你的元素应该仅通过解析自己的内容提供它知道的数据，而不是另一个元素的内容。例如，AVI头部在头部提供了包含音频流的采样率。而 MPEG 系统流则不提供。这意味着AVI流解复用器会将采样率作为 MPEG 音频流的属性提供，而 MPEG 解复用器不会。需要这些数据的解码器需要在其中插入一个流解析器来从头部提取这些数据或从流中计算它。

### 3.1 Table of Audio Types

### 3.2 Table of Video Types

### 3.3 Table of Container Types

### 3.4 Table of Subtitle Types

### 3.5 Table of Other Types

## 参考
[翻译自：Media Types and Properties](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/media-types.html?gi-language=c#media-types-and-properties)
