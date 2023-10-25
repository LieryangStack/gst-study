# Constructing the Boilerplate

在本章中，你将学习如何为一个新插件构建最少的代码。从起点开始，您将看到如何获取GStreamer模板源代码。然后，您将学习如何使用一些基本工具来复制和修改模板插件，以创建一个新的插件。如果您遵循这里的示例，那么在本章结束时，您将拥有一个可以编译并在GStreamer应用程序中使用的功能强大的音频过滤器插件。

## Getting the GStreamer Plugin Templates

目前有两种方法可以开发GStreamer的新插件:可以手工编写整个插件，也可以复制现有的插件模板并编写所需的插件代码。到目前为止，第二种方法是两种方法中比较简单的一种，因此在这里就不介绍第一种方法了。(嗯，也就是说，“留给读者做练习。”)

第一步是检出gst-template git模块的一个副本，以获取一个重要的工具和一个基本GStreamer插件的源代码模板。要查看gst-template模块，请确保已连接到互联网，然后在命令控制台中输入以下命令:

```bash
shell $ git clone https://gitlab.freedesktop.org/gstreamer/gst-template.git
Initialized empty Git repository in /some/path/gst-template/.git/
remote: Counting objects: 373, done.
remote: Compressing objects: 100% (114/114), done.
remote: Total 373 (delta 240), reused 373 (delta 240)
Receiving objects: 100% (373/373), 75.16 KiB | 78 KiB/s, done.
Resolving deltas: 100% (240/240), done.
```

这个命令会检出一系列文件和目录到gst-template中。我们要使用的模板在gst-template/gst-plugin/目录下。你应该查看该目录下的文件，以对插件源代码树的结构有一个大致的了解。

如果由于某种原因您无法访问git仓库，您也可以通过[gitlab web](https://gitlab.freedesktop.org/gstreamer/gst-template)界面下载最新版本的快照。

## Using the Project Stamp

创建新元素时要做的第一件事是指定一些基本细节:它的名称、编写它的人、它的版本号等。我们还需要定义一个对象来表示元素并存储元素所需的数据。这些细节统称为样板(boilerplate)。

定义样板代码的标准方法很简单，就是编写一些代码，然后填充一些结构。如上一节所述，最简单的方法是复制模板并根据需要添加功能。在./gst-plugin/tools/目录下有一个工具可以帮助你做到这一点。这个工具make_element是一个命令行实用工具，它为您创建样板代码。

要使用`make_element`，首先要打开一个终端窗口。切换到目录`gst-template/gst-plugin/src`，然后执行`make_element`命令。`make_element`的参数如下:

- 1.插件的名称

- 2.工具将使用的源文件。默认使用gstplugin。

例如，下面的命令根据插件模板创建MyFilter插件，并将输出文件放在目录gst-template/gst-plugin/src中:

```bash
cd gst-template/gst-plugin/src
../tools/make_element MyFilter
```

**注意：**	

 1. 对于插件的名称，大小写很重要。请记住，在某些操作系统下，通常指定目录和文件名时，大小写也很重要。最后一个命令创建两个文件:gstmyfilter.c和gstmyfilter.h。
 2. 建议您先创建一个gst-plugin目录的副本再继续下一步操作。现在需要从父目录运行meson build来引导构建环境。之后，可以使用众所周知的ninja -C build命令来构建和安装项目。
 3. 请注意，在默认情况下，meson将选择/usr/local作为默认位置。我们需要将/usr/local/lib/gstreamer-1.0添加到GST_PLUGIN_PATH中，以便让新插件显示gstreamer安装列表中。
 4. FIXME:这一节有点过时了。作为一个最小插件构建系统框架的例子，Gst-template仍然很有用。但是，对于创建元素，现在推荐使用gst-plugins-bad中的gst-element-maker工具。

## Examining the Basic Code

首先，我们将检查可能放在头文件中的代码(尽管由于代码的接口完全由插件系统定义，不依赖于读取头文件，因此这不是至关重要的)。

首先，我们将检查可能放在头文件中的代码(尽管由于代码的接口完全由插件系统定义，不依赖于读取头文件，因此这不是至关重要的)。
```c
#include <gst/gst.h>

/* Definition of structure storing data for this element. */
typedef struct _GstMyFilter {
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;



} GstMyFilter;

/* Standard definition defining a class for this element. */
typedef struct _GstMyFilterClass {
  GstElementClass parent_class;
} GstMyFilterClass;

/* Standard macros for defining types for this element.  */
#define GST_TYPE_MY_FILTER (gst_my_filter_get_type())
#define GST_MY_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MY_FILTER,GstMyFilter))
#define GST_MY_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MY_FILTER,GstMyFilterClass))
#define GST_IS_MY_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MY_FILTER))
#define GST_IS_MY_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MY_FILTER))

/* Standard function returning type information. */
GType gst_my_filter_get_type (void);

GST_ELEMENT_REGISTER_DECLARE(my_filter)


```
使用这个头文件，你可以使用以下宏在源文件中设置元素基础，以便正确调用所有函数:

```c
#include "filter.h"

G_DEFINE_TYPE (GstMyFilter, gst_my_filter, GST_TYPE_ELEMENT);
GST_ELEMENT_REGISTER_DEFINE(my_filter, "my-filter", GST_RANK_NONE, GST_TYPE_MY_FILTER);

```

宏GST_ELEMENT_REGISTER_DEFINE与GST_ELEMENT_REGISTER_DECLARE结合使用，允许在插件内部或通过调用GST_ELEMENT_REGISTER (my_filter)从任何其他插件/应用程序注册元素。

## Element metadata

元素元数据提供额外的元素信息。它配置了gst_element_class_set_metadata或gst_element_class_set_static_metadata，参数如下:
 - 元素的英文长名称。
 - 元素的类型，请参阅GStreamer核心源代码树中的docs/additional/design/draft-klass.txt文档了解详细信息和示例。
 - 元素用途的简要描述。
 - 元素作者的名字，后面可以加上尖括号中的联系电子邮件地址。

例如:

```c
gst_element_class_set_static_metadata (klass,
  "An example plugin",
  "Example/FirstExample",
  "Shows the basic structure of a plugin",
  "your name <your.name@your.isp>");

```
元素的细节是在_class_init()函数期间注册到插件的，这个函数是GObject系统的一部分。在向GLib注册类型的函数中，应该为这个GObject设置_class_init()函数。

```c
static void
gst_my_filter_class_init (GstMyFilterClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

[..]
  gst_element_class_set_static_metadata (element_class,
    "An example plugin",
    "Example/FirstExample",
    "Shows the basic structure of a plugin",
    "your name <your.name@your.isp>");

}

```

## GstStaticPadTemplate

GstStaticPadTemplate是元素将(或可能)创建和使用的pad
的描述。它包含:
 - pad的简称。
 - pad的方向。
 - pad存在的属性（请求、始终、有时）。这表明该pad是否始终存在(always垫)，仅在某些情况下存在(sometimes垫)，或者仅在应用程序请求这样一个垫(request垫)时存在。

此元素支持的类型(功能)。

例如:
```c
static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);

```
这些pad模板是在_class_init()函数中使用gst_element_class_add_pad_template()注册的。对于这个函数，您需要一个处理GstPadTemplate，您可以从静态pad模板创建与gst_static_pad_template_get()。有关此的更多细节请参见下文。

元素的_init()函数使用gst_pad_new_from_static_template()从这些静态模板创建pad。为了使用gst_pad_new_from_static_template()从这个模板创建一个新的pad，需要将pad模板声明为全局变量。有关此主题的更多信息请参见指定pad。

```c
static GstStaticPadTemplate sink_factory = [..],
    src_factory = [..];

static void
gst_my_filter_class_init (GstMyFilterClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
[..]

  gst_element_class_add_pad_template (element_class,
    gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
    gst_static_pad_template_get (&sink_factory));
}

```

模板中的最后一个参数是其类型或支持的类型列表。在这个例子中，我们使用了` ANY `，这意味着这个元素将接受所有输入。在实际情况中，您可以设置一个媒体类型和可选的一组属性，以确保只接收受支持的输入。这种表示应该是一个字符串，以媒体类型开始，然后是一组以逗号分隔的属性及其支持的值。如果音频滤波器支持任意采样的原始整数16位音频、单声道或立体声，正确的模板应该是这样的:

```c
static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (
    "audio/x-raw, "
      "format = (string) " GST_AUDIO_NE (S16) ", "
      "channels = (int) { 1, 2 }, "
      "rate = (int) [ 8000, 96000 ]"
  )
);

```
被大括号("{"和"}")括起来的值是列表，被方括号("["和"]")括起来的值是范围。它还支持多组类型，并且应该用分号(“;”)分隔。在关于pad的这一章中，我们将看到如何使用类型来知道流的确切格式:指定pad。

## Constructor Functions

每个元素都有两个函数，用于构造元素。_class_init()函数，只用于初始化类一次(指定类有哪些信号、参数和虚函数，并设置全局状态);以及_init()函数，该函数用于初始化该类型的特定实例。

## The plugin_init function

编写了定义插件所有部分的代码后，还需要编写plugin_init()函数。这是一个特殊的函数，插件加载后就会调用它，返回TRUE或FALSE，取决于插件是否正确地加载并初始化了任何依赖项。此外，在这个函数中，应该注册插件中任何受支持的元素类型。

```c
static gboolean
plugin_init (GstPlugin *plugin)
{
  return GST_ELEMENT_REGISTER (my_filter, plugin);
}

GST_PLUGIN_DEFINE (
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  my_filter,
  "My filter plugin",
  plugin_init,
  VERSION,
  "LGPL",
  "GStreamer",
  "http://gstreamer.net/"
)
```
请注意，plugin_init()函数返回的信息将缓存在一个中央注册表中。因此，函数总是返回相同的信息是很重要的:例如，它不能根据运行时条件使元素factory可用。如果一个元素只能在某些条件下工作(例如，如果声卡没有被其他进程使用)，这必须反映为该元素在不可用时无法进入就绪状态，而不是插件试图否认该插件的存在。


## 参考
[翻译自：Constructing the Boilerplate](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/boiler.html?gi-language=c)
