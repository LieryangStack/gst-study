# 插件开发介绍
## 1 前言
### 1.1 什么是Gstreamer？

GStreamer是用于创建流媒体应用程序的框架。其基本设计灵感来自于俄勒冈研究生院的视频管道，同时也借鉴了DirectShow的一些思想。

GStreamer的开发框架使得编写任何类型的流媒体应用程序成为可能。GStreamer框架的设计旨在轻松编写处理音频、视频或两者兼顾的应用程序。它并不局限于音频和视频，还可以处理任何类型的数据流。其管道设计旨在使得应用的过滤器所引起的额外开销尽可能小。这使得GStreamer成为设计高要求低延迟或高性能的高端音频应用程序的良好框架。

>注释1：
<span style="background-color: lightgreen;">其管道设计旨在使得应用的过滤器所引起的额外开销尽可能小</span>

><span style="background-color: lightgreen;">我的理解：pipeline设计是为了让中间插件(filters)引起的开销足够的小。</span>

GStreamer最显而易见的用途之一是用它来构建媒体播放器。GStreamer已经包含了用于构建支持各种各样格式的媒体播放器的组件，包括MP3、Ogg/Vorbis、MPEG-1/2、AVI、Quicktime、mod等等。然而，GStreamer不仅仅是又一个媒体播放器。它的主要优势在于可插拔的组件可以混合匹配成任意的管道，因此可以编写完整的视频或音频编辑应用程序。

该框架基于插件，这些插件提供了各种编解码器和其他功能。这些插件可以链接并排列在一个管道中，这个管道定义了数据的流动方式。

GStreamer的核心功能包括提供插件、数据流、同步以及媒体类型处理/协商的框架。它还提供了一个API，用于编写使用各种插件的应用程序。

### 1.2 谁应该读该指南？

这份指南解释了如何为GStreamer编写新的模块。该指南与以下几类人群相关：

 - 任何希望为GStreamer添加对新的数据处理方式的支持的人。例如，该组中的人可能希望创建新的数据格式转换器、新的可视化工具，或者新的解码器或编码器。

 - 任何希望为新的输入和输出设备添加支持的人。例如，该组中的人可能希望添加将数据写入新的视频输出系统或从数字相机或特殊麦克风中读取数据的功能。

 - 任何希望以任何方式扩展GStreamer的人。在理解插件系统如何工作之前，您需要了解插件系统对其余代码的限制。<span style="background-color: lightgreen;">`我的理解：`在GStreamer或类似的系统中，插件系统通常用于扩展和定制软件的功能。这些插件是模块化的，可以动态加载和卸载，以便添加新功能或修改现有功能。然而，插件系统通常会引入一些规则和约束，以确保插件的正确集成和协同工作。因此，这句话的要点是，了解插件系统的工作原理是理解插件系统如何影响和限制代码的前提条件。只有理解了插件系统的机制，才能更好地理解为何需要在编写新模块或扩展功能时遵守特定的规则和约定，以确保系统的稳定性和可维护性。</span>。此外，在阅读本指南后，您可能会惊讶于插件可以实现的功能的多样性。

如果您只想使用GStreamer的现有功能，或者只想使用使用了GStreamer的应用程序，那么这份指南对您不相关。如果您只对使用现有插件来编写新应用程序感兴趣（已经有相当多的插件了），您可能需要查看GStreamer应用程序开发手册。如果您只是试图获取有关GStreamer应用程序的帮助，那么您应该查看特定应用程序的用户手册。

### 1.3 预先阅读

这份指南假定你对GStreamer的基本运作方式有一定了解。如果你想对GStreamer中的编程概念有一定的了解，建议你首先阅读《GStreamer应用程序开发手册》。此外，你还可以查看GStreamer网站上提供的其他文档。

为了理解本手册，你需要对C语言有基本的了解。由于GStreamer遵循GObject编程模型，本指南还假定你了解GObject编程的基础知识。你可能还想看一下Eric Harlow的书《使用GTK+和GDK开发Linux应用程序》。

### 1.4 指南（Guide）的结构

为了帮助你更好地浏览本指南，它被分为几个大部分。每个部分都涵盖了与GStreamer插件开发相关的特定广泛主题。本指南的各部分按以下顺序排列：

 - [Building a Plugin](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/index.html?gi-language=c) - 这部分涵盖了通常需要执行的构建插件的基本步骤，例如在GStreamer中注册元素以及设置基本配置，以便它能够接收来自相邻元素的数据并发送数据给它们。讨论从构建模板和在“构建模板”部分中注册元素的示例开始。然后，你将学习如何编写代码以使基本filter插件正常工作，包括指定pad、chain function以及state的介绍。

   接下来，我们将展示一些关于如何使元素适用于应用程序并在应用程序和元素之间进行交互的GObject概念，详见“添加属性和信号”部分。然后，你将学习构建一个快速测试应用程序，以测试你刚刚学到的内容，这部分见“构建测试应用程序”。在这里，我们只涉及基础知识。对于全面的应用程序开发，请查看应用程序开发手册。

 - [Advanced Filter Concepts](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/index.html?gi-language=c) - 关于GStreamer插件开发的高级特性信息。
  
   在了解了基本步骤之后，你应该能够创建一个具有一些不错功能的音频或视频滤波(filter)插件。然而，GStreamer为插件开发者提供了更多功能。本指南的这一部分包括关于更高级主题的章节，例如scheduling、GStreamer中的媒体类型定义、时钟、接口和tagging。由于这些功能是针对特定目的的，你可以以任何顺序阅读它们，大多数章节不需要来自其他部分的知识。

   第一章，命名为[Different scheduling modes](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/scheduling.html?gi-language=c)，将解释元素调度的一些基础知识。它不是非常深入，但主要是一种介绍，解释为什么其他事情以它们的方式工作。如果你对GStreamer内部机制感兴趣，可以阅读这一章。接下来，我们将应用这些知识，[The chain function: Different scheduling modes](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/chainfn.html?gi-language=c)中学到的数据传输类型。Loop-based元素将使你对输入速率有更多控制。这在编写复用器或解复用器等情况下非常有用。

   接下来，我们将在“媒体类型和属性”中讨论GStreamer中的媒体识别。你将学会如何定义新的媒体类型，并了解GStreamer中定义的标准媒体类型列表。

   在下一章中，你将了解请求和有时的插口的概念，这些插口是动态创建的，要么是因为应用程序请求（请求），要么是因为媒体流需要（有时）。这将在"Request and Sometimes pads"。

   接下来的章节，时钟，将解释GStreamer中时钟的概念。当你想要了解元素如何实现音频/视频同步时，就需要这些信息。

   接下来的几章将讨论应用程序与元素互动的高级方式。之前，我们学习了在“添加属性和信号”中使用GObject的方式来实现这一点。我们将讨论动态参数，这是一种预先定义元素随时间行为的方式，见“支持动态参数”。接下来，你将学习有关接口的概念，见“接口”。接口是非常特定于目标的应用程序-元素交互方式，基于GObject的GInterface。最后，你将了解GStreamer中如何处理元数据，在“ Tagging (Metadata and Streaminfo)”中。

   最后一章， Events: Seeking, Navigation and More,，将讨论GStreamer中事件的概念。事件是进行应用程序-元素交互的另一种方式。例如：seeking事件。它们还是元素相互交互的另一种方式，例如让彼此了解媒体流的不连续性，在管道内转发标记等等。

 - [Creating special element types](https://gstreamer.freedesktop.org/documentation/plugin-development/element-types/index.html?gi-language=c)  -  解释如何编写其他插件类型。

   因为指南的前两部分以音频滤波器作为示例，所介绍的概念适用于滤波器插件。但许多概念同样适用于其他插件类型，包括sources, sinks, and autopluggers。本指南的这一部分介绍了在处理这些更专业的插件类型时可能出现的问题。该章节首先重点关注可以使用基类编写的元素（预制基类），然后还会涉及编写特殊类型的元素，如 Writing a Demuxer or Parser, Writing a N-to-1 Element or Muxer and Writing a Manager。
 
 - [Appendices](https://gstreamer.freedesktop.org/documentation/plugin-development/appendix/index.html?gi-language=c) -  给插件开发者的进一步信息。

   附录包含了一些信息，它们不太容易整洁地放在指南的其他部分中。本部分的大部分内容尚未完成。


指南的这一部分剩余部分将简要概述了GStreamer插件开发涉及的基本概念。涵盖的主题包括元素和插件、插口、GstMiniObject、缓冲区和事件以及媒体类型和属性。如果你已经熟悉这些信息，你可以使用这个简要概述来温习知识，或者你可以跳到“构建插件”。

正如你所看到的，有很多东西要学，所以让我们开始吧！

  - 通过扩展GstBin来创建复合和复杂元素。这将允许你创建包含其他插件的插件。

  - 向注册表添加新的媒体类型以及类型检测函数。这将允许你的插件在全新的媒体类型上运行。


## 参考
[一、翻译自Gstreamer官网Plugin Writer`s Guide/Introduction](https://gstreamer.freedesktop.org/documentation/plugin-development/introduction/preface.html?gi-language=c)