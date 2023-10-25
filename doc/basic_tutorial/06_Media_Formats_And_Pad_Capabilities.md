## 目标
Pad功能（Capabilities）是GStreamer的一个基本元素，尽管大多数时候它们是不可见的，因为框架会自动处理它们。这个有点理论化的教程展示了:

 - 什么是Pad能力。
 - 如何检索它们。
 - 何时检索它们。
 - 你为什么要知道他们。

## 介绍
### Pads
如前所示，Pads允许信息进入和离开元素。然后，Pad的Capabilities功能(或简称Caps)指定了哪些类型的信息可以通过Pad传递。例如，“分辨率为320x200像素，每秒30帧的RGB视频”，或“每个音频样本为16位，5.1通道，每秒44100个样本”，或甚至像mp3或h264这样的压缩格式。

pad可以支持多种功能(例如，一个视频sink可以支持不同类型的RGB或YUV格式的视频)，功能可以指定为范围(例如，一个音频sink可以支持每秒1到48000个样本的采样率)。但是，从一个Pad传输到另一个Pad的实际信息必须只有一种明确指定的类型。通过一个称为协商的过程，两个连接的pads商定一个共同的类型，因此Pads的能力变得固定(它们只有一种类型，不包含范围)。下面的示例代码应该能让这一切变得清晰。

`为了将两个元素链接在一起，它们必须共享一个共同的功能子集`(否则它们就不可能相互理解)。这是Capabilities的主要目的。

作为应用程序开发人员，你通常会通过将元素链接在一起来构建管道(如果你使用像playbin这样的all-in-all元素，则会在较小的程度上进行链接)。在这种情况下，你需要知道元素的Pad Caps，或者至少知道当GStreamer拒绝将两个元素链接起来并报错是什么。
### Pad templates
Pads是从Pad Templates创建的，模板表示Pad可能具有的所有功能。模板可用于创建多个相似的Pad，还允许元素之间的连接提前拒绝:如果它们的Pad模板的功能没有一个共同的子集(它们的交集为空)，则没有必要进一步协商。

Pad模板可以看作是协商过程中的第一步。随着流程的发展，实际的pad会被实例化，它们的功能会被细化，直到它们被确定(或者协商失败)。

## Capabilities examples
```bash
SINK template: 'sink'
  Availability: Always
  Capabilities:
    audio/x-raw
               format: S16LE
                 rate: [ 1, 2147483647 ]
             channels: [ 1, 2 ]
    audio/x-raw
               format: U8
                 rate: [ 1, 2147483647 ]
             channels: [ 1, 2 ]

```

这个pad是一个sink，总是可以在元素上使用(我们现在不会讨论可用性)。它支持两种媒体，都是整数格式的原始音频(audio/x-raw):有符号的16位小端序和无符号的8位。方括号表示一个范围，例如通道数从1到2。

```bash
SRC template: 'src'
  Availability: Always
  Capabilities:
    video/x-raw
                width: [ 1, 2147483647 ]
               height: [ 1, 2147483647 ]
            framerate: [ 0/1, 2147483647/1 ]
               format: { I420, NV12, NV21, YV12, YUY2, Y42B, Y444, YUV9, YVU9, Y41B, Y800, Y8, GREY, Y16 , UYVY, YVYU, IYU1, v308, AYUV, A420 }
```
video/x-raw表示该src pad输出原始视频。它支持各种尺寸和帧速率，以及一组YUV格式(大括号表示列表)。所有这些格式都表示图像平面的不同打包和下采样。

## Last remarks
您可以使用基本教程10:GStreamer工具中描述的gst-inspect-1.0工具来学习任何GStreamer元素的Caps。

请记住，有些成员会查询底层硬件以获取支持的格式，并相应地提供其Pad cap(通常在进入就绪状态或更高状态时这样做)。因此，所示的caps可能因平台而异，甚至可能因执行不同而异(尽管这种情况很少见)。

本教程实例化两个元素(这次，通过它们的工厂)，显示它们的Pad模板，链接它们并设置管道播放。在每次状态变化时，都会显示sink元素的Pad的能力，因此可以观察协商是如何进行的，直到Pad Caps确定。

## A trivial Pad Capabilities Example
```c
#include <gst/gst.h>

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    g_print ("%sANY\n", pfx);
    return;
  }
  if (gst_caps_is_empty (caps)) {
    g_print ("%sEMPTY\n", pfx);
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);

    g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
    gst_structure_foreach (structure, print_field, (gpointer) pfx);
  }
}

/* Prints information about a Pad Template, including its Capabilities */
static void print_pad_templates_information (GstElementFactory * factory) {
  const GList *pads;
  GstStaticPadTemplate *padtemplate;

  g_print ("Pad Templates for %s:\n", gst_element_factory_get_longname (factory));
  if (!gst_element_factory_get_num_pad_templates (factory)) {
    g_print ("  none\n");
    return;
  }

  pads = gst_element_factory_get_static_pad_templates (factory);
  while (pads) {
    padtemplate = pads->data;
    pads = g_list_next (pads);

    if (padtemplate->direction == GST_PAD_SRC)
      g_print ("  SRC template: '%s'\n", padtemplate->name_template);
    else if (padtemplate->direction == GST_PAD_SINK)
      g_print ("  SINK template: '%s'\n", padtemplate->name_template);
    else
      g_print ("  UNKNOWN!!! template: '%s'\n", padtemplate->name_template);

    if (padtemplate->presence == GST_PAD_ALWAYS)
      g_print ("    Availability: Always\n");
    else if (padtemplate->presence == GST_PAD_SOMETIMES)
      g_print ("    Availability: Sometimes\n");
    else if (padtemplate->presence == GST_PAD_REQUEST) {
      g_print ("    Availability: On request\n");
    } else
      g_print ("    Availability: UNKNOWN!!!\n");

    if (padtemplate->static_caps.string) {
      GstCaps *caps;
      g_print ("    Capabilities:\n");
      caps = gst_static_caps_get (&padtemplate->static_caps);
      print_caps (caps, "      ");
      gst_caps_unref (caps);

    }

    g_print ("\n");
  }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  if (!pad) {
    g_printerr ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}

int main(int argc, char *argv[]) {
  GstElement *pipeline, *source, *sink;
  GstElementFactory *source_factory, *sink_factory;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the element factories */
  source_factory = gst_element_factory_find ("audiotestsrc");
  sink_factory = gst_element_factory_find ("autoaudiosink");
  if (!source_factory || !sink_factory) {
    g_printerr ("Not all element factories could be created.\n");
    return -1;
  }

  /* Print information about the pad templates of these factories */
  print_pad_templates_information (source_factory);
  print_pad_templates_information (sink_factory);

  /* Ask the factories to instantiate actual elements */
  source = gst_element_factory_create (source_factory, "source");
  sink = gst_element_factory_create (sink_factory, "sink");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new ("test-pipeline");

  if (!pipeline || !source || !sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
  if (gst_element_link (source, sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Print initial negotiated caps (in NULL state) */
  g_print ("In NULL state:\n");
  print_pad_capabilities (sink, "sink");

  /* Start playing */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state (check the bus for error messages).\n");
  }

  /* Wait until error, EOS or State Change */
  bus = gst_element_get_bus (pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
        GST_MESSAGE_STATE_CHANGED);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("\nPipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
            /* Print the current capabilities of the sink element */
            print_pad_capabilities (sink, "sink");
          }
          break;
        default:
          /* We should not reach here because we only asked for ERRORs, EOS and STATE_CHANGED */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  gst_object_unref (source_factory);
  gst_object_unref (sink_factory);
  return 0;
}

```

## 解释
print_field、print_caps和print_pad_templates只是以人类友好的格式显示能力结构。如果您想了解GstCaps结构的内部组织，请阅读有关Pad Caps的GStreamer文档。

```c
/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  if (!pad) {
    g_printerr ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}

```
gst_element_get_static_pad()从给定的元素中取得指定的Pad这个Pad是静态的，因为它总是存在于元素中。要了解有关Pad可用性的更多信息，请阅读有关Pad的GStreamer文档。

然后调用gst_pad_get_current_caps()检索Pad的当前能力，该能力是否固定取决于协商过程的状态。在这种情况下，我们可以调用gst_pad_query_caps()来获取当前可接受的Pad Capabilities。当前可接受的Caps将是Pad Template`s Caps，在NULL状态下的Caps。但可能在以后的状态中发生变化，因为实际的硬件能力可能会被查询。

然后打印这些功能。
```c
/* Create the element factories */
source_factory = gst_element_factory_find ("audiotestsrc");
sink_factory = gst_element_factory_find ("autoaudiosink");
if (!source_factory || !sink_factory) {
  g_printerr ("Not all element factories could be created.\n");
  return -1;
}

/* Print information about the pad templates of these factories */
print_pad_templates_information (source_factory);
print_pad_templates_information (sink_factory);

/* Ask the factories to instantiate actual elements */
source = gst_element_factory_create (source_factory, "source");
sink = gst_element_factory_create (sink_factory, "sink");

```
在前面的教程中，我们直接使用gst_element_factory_make()创建元素，并跳过了工厂的讨论，但我们现在将这样做。GstElementFactory负责实例化一个特定类型的元素，通过它的工厂名称标识。

你可以使用gst_element_factory_find()来创建一个“videotestsrc”类型的工厂，然后使用gst_element_factory_create()来实例化多个“videotestsrc”元素。gst_element_factory_make()实际上是gst_element_factory_find()+ gst_element_factory_create()的快捷方式。

Pad模板已经可以通过工厂访问，因此一旦创建工厂，它们就会被打印出来。

我们跳过管道的创建并开始，转到状态改变的消息处理:
```c
case GST_MESSAGE_STATE_CHANGED:
  /* We are only interested in state-changed messages from the pipeline */
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    g_print ("\nPipeline state changed from %s to %s:\n",
        gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
    /* Print the current capabilities of the sink element */
    print_pad_capabilities (sink, "sink");
  }
  break;

```
这只是在每次管道状态改变时打印当前的Pad Caps。在输出中，您应该看到初始的caps (Pad模板的caps)是如何逐步细化直到它们完全固定(它们包含一个没有范围的单一类型)。

## 总结
本教程展示了:

 - 什么是Pad Capabilities和Pad Template Capabilities。
 - 如何使用gst_pad_get_current_caps()或gst_pad_query_caps()取得它们。
 - 根据管道状态的不同，它们具有不同的含义(最初它们表示所有可能的Caps，后来它们表示当前协商的Pad Caps)。
 - 事先知道两个元素Pad Caps是否可以连接在一起是很重要的。
 - 可以使用基本教程10:GStreamer工具中描述的gst-inspect-1.0工具找到Pad Caps。

下一个教程展示了如何手动将数据注入到GStreamer管道中并从管道中提取。

请记住，在这个页面的附件中，您应该可以找到本教程的完整源代码和构建它所需的任何附件文件。很高兴在这里见到你，希望很快见到你!

## 关于GstPad、GstCaps、GstStructure

 * print_pad_capabilities(GstElement *element, gchar *pad_name) 通过element和pad_name 得到 GstCaps
 * print_caps(const GstCaps *caps, const gchar *pfx) 遍历GstCaps里面的每个GstStructure
 * print_field(GQuark field, const GValue *value, gpointer pfx) 打印每个GstStructure里面的内容

例如：
```bash
Caps for the sink pad:
         video/x-raw
                   format: YUY2
                    width: [ 1, 32768 ]
                   height: [ 1, 32768 ]
                framerate: [ 0/1, 2147483647/1 ]
         video/x-raw
                   format: YV12
                    width: [ 1, 32768 ]
                   height: [ 1, 32768 ]
                framerate: [ 0/1, 2147483647/1 ]
         video/x-raw
                   format: UYVY
                    width: [ 1, 32768 ]
                   height: [ 1, 32768 ]
                framerate: [ 0/1, 2147483647/1 ]
         video/x-raw
                   format: I420
                    width: [ 1, 32768 ]
                   height: [ 1, 32768 ]
                framerate: [ 0/1, 2147483647/1 ]

```
>这就是一个GstStructure
> video/x-raw
                   format: YUY2
                    width: [ 1, 32768 ]
                   height: [ 1, 32768 ]
                framerate: [ 0/1, 2147483647/1 ]