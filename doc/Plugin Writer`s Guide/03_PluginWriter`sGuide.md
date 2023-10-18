# 3 Plugin Writer`s Guide

现在，您已经准备好学习如何构建一个插件了。在本指南的这一部分，您将学习如何将基本的GStreamer编程概念应用于编写一个简单的插件。指南的前几部分没有包含明确的示例代码，这可能使事情有点抽象和难以理解。相比之下，这一部分将通过开发一个名为"MyFilter"的示例音频滤波插件来展示应用和代码。

示例的滤波器元素将从一开始只有一个输入Pad和一个输出Pad。该滤波器起初将简单地将其汇Pad和源Pad上的媒体和事件数据传递，而不进行修改。但在本指南的这一部分结束时，您将学会添加一些更有趣的功能，包括属性和信号处理程序。并在阅读下一部分指南"[高级滤波器概念](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/index.html?gi-language=c)"后，您将能够为您的插件添加更多功能。

Constructing the Boilerplate

Specifying the pads

The chain function

The event function

The query function

What are states?

Adding Properties

Signals

Building a Test Application



01 Request and Sometimes pads

02 Different scheduling modes

03 Caps negotiation

04 Memory allocation

05 Media Types and Properties

06 Events: Seeking, Navigation and More

07 Clocking

08 Quality Of Service (QoS)

09 Supporting Dynamic Parameters

10 Interfaces

11 Tagging (Metadata and Streaminfo)

