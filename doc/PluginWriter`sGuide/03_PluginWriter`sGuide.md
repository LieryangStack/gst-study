# 3 Plugin Writer`s Guide

现在，您已经准备好学习如何构建一个插件了。在本指南的这一部分，您将学习如何将基本的GStreamer编程概念应用于编写一个简单的插件。指南的前几部分没有包含明确的示例代码，这可能使事情有点抽象和难以理解。相比之下，这一部分将通过开发一个名为"MyFilter"的示例音频滤波插件来展示应用和代码。

示例的滤波器元素将从一开始只有一个输入Pad和一个输出Pad。该滤波器起初将简单地将其汇Pad和源Pad上的媒体和事件数据传递，而不进行修改。但在本指南的这一部分结束时，您将学会添加一些更有趣的功能，包括属性和信号处理程序。并在阅读下一部分指南"[高级滤波器概念](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/index.html?gi-language=c)"后，您将能够为您的插件添加更多功能。

[01 Constructing the Boilerplate](./03_PluginWriter`sGuide/01_Constructing_the_Boilerplate.md)

[02 Specifying the pads](./03_PluginWriter`sGuide/02_Specifying_the_pads.md)

[03 The chain function](./03_PluginWriter`sGuide/03_The_chain_function.md)

[04 The event function](./03_PluginWriter`sGuide/04_The_event_function.md)

[05 The query function](./03_PluginWriter`sGuide/05_The_query_function.md)

[06 What are states?](./03_PluginWriter`sGuide/06_What_are_states.md)

[07 Adding Properties](./03_PluginWriter`sGuide/07_Adding_Properties.md)

[08 Signals](./03_PluginWriter`sGuide/08_Signals.md)

[09 Building a Test Application](./03_PluginWriter`sGuide/09_Building_a_Test_Application.md)


