# 3 Caps negotiation

Caps协商是在元素之间寻找它们可以处理的媒体格式（GstCaps）的过程。在大多数情况下，GStreamer 中的这个过程可以找到完整管道的最佳解决方案。在本节中，我们将解释这个过程是如何工作的。

## Cpas negotiation basics

在GStreamer中，媒体格式的协商始终遵循以下简单规则：

- 下游元素建议一种格式在其sinkpad上，并将这一建议放置在对下游sinkpad执行的CAPS查询的结果中。也可参见“Implementing a CAPS query function”。

- 上游元素决定一种格式，这个上游元素并通过CAPS事件将所选的媒体格式发送到其source pad下游元素。下游元素重新配置自身以处理CAPS事件中的媒体类型。

- 下游元素可以通过向上游发送RECONFIGURE事件来通知上游，表明它希望建议一种新格式。RECONFIGURE事件简单地指示上游元素重新启动协商阶段。因为发送RECONFIGURE事件的元素现在建议另一种格式，所以管道中的格式可能会发生变化。

除了CAPS和RECONFIGURE事件以及CAPS查询之外，还有ACCEPT_CAPS查询，用于快速检查元素是否可以接受某种caps。

所有协商都遵循这些简单规则。让我们看一些典型的用例以及协商的进行方式。

## Caps negotiation use cases

接下来，我们将看一些push-mode调度的用例。pull-mode调度协商阶段在“Pull-mode Caps negotiation”中讨论，实际上与我们将看到的类似。

由于sink pads仅建议格式，<span style="background-color: pink;">而source pad需要做出决定</span>，因此最复杂的工作在source pad中完成。我们可以为source pad识别三种caps协商的用例：

- 固定协商Fixed negotiation。一个元素只能输出一种格式。请参见“Fixed negotiation”。

- 转换协商Transform negotiation。元素的输入和输出格式之间存在（固定的）转换，通常基于某个元素属性。元素将产生的caps取决于上游caps，元素可以接受的caps取决于下游caps。请参见“Transform negotiation”。

- 动态协商Dynamic negotiation。一个元素可以输出多种格式。请参见“Dynamic negotiation”。

### Fixed negotiation

在这种情况下，source pad只能生成一种固定格式。通常，这种格式已经编码在媒体内部。没有下游元素可以请求不同的格式，source pad重新协商的唯一方式是当元素决定自行更改caps时。

通常情况下，可以实现固定caps（在其source pad上）的元素是所有不可重新协商的元素。例如：

- 类型查找器，因为找到的类型是实际数据流的一部分，因此不能重新协商。类型查找器将检查字节流，确定类型，发送带有caps的CAPS事件，然后推送该类型的缓冲区。

- 几乎所有的demuxers，因为包含的基本数据流在文件头中已经定义，因此无法重新协商。

- 一些解码器，其中格式嵌入在数据流中，不是peercaps的一部分，而且解码器本身也不可重新配置。

- 一些生成固定格式的源。

对于具有固定caps的source pad，使用gst_pad_use_fixed_caps()。只要垫没有进行协商，默认的CAPS查询将返回垫模板中呈现的caps。一旦垫被协商，CAPS查询将返回协商的caps（而不是其他任何内容）。以下是固定caps source pad的相关代码片段。

```c
[..]
  pad = gst_pad_new_from_static_template (..);
  gst_pad_use_fixed_caps (pad);
[..]
```
可以通过调用gst_pad_set_caps()将固定的caps设置到pad上。
```c
[..]
    caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, GST_AUDIO_NE(F32),
        "rate", G_TYPE_INT, <samplerate>,
        "channels", G_TYPE_INT, <num-channels>, NULL);
    if (!gst_pad_set_caps (pad, caps)) {
      GST_ELEMENT_ERROR (element, CORE, NEGOTIATION, (NULL),
          ("Some debug information here"));
      return GST_FLOW_ERROR;
    }
[..]
```

这些类型的元素也没有输入格式和输出格式之间的关系，输入caps简单地不包含生成输出caps所需的信息。

所有其他需要为格式进行配置的元素应该实现完整的caps协商，这将在接下来的几节中进行解释。


### Transform negotiation

### Dynamic negotiation

## Upstream caps (re)negotiation

## Implementing a CAPS query function

## Pull-mode Caps negotiation

## 参考

[翻译自：Caps negotiation](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/negotiation.html?gi-language=c)