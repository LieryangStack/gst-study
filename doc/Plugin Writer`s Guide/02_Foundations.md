# 基本概念
本指南的这一章介绍了GStreamer的基本概念。理解这些概念将有助于你理解扩展GStreamer涉及的问题。其中许多概念在GStreamer应用程序开发手册中有更详细的解释；这里介绍的基本概念主要用于温习你的记忆。

## 1 Elements and Plugins

元素是GStreamer的核心。在插件开发的上下文中，元素是从GstElement类派生的对象。当与其他元素链接时，元素提供某种功能：例如，源元素为流提供数据，而过滤元素对流中的数据进行操作。没有元素，GStreamer只是一堆概念上的管道配件，没有可链接的东西。GStreamer附带了大量的元素，但也可以编写额外的元素。

然而，仅仅编写一个新元素还不够：您需要将您的元素封装在一个插件中，以便GStreamer可以使用它。插件本质上是一个可加载的代码块，通常称为共享对象文件或动态链接库。一个插件可能包含多个元素的实现，也可能只包含一个元素。出于简单起见，本指南主要集中在包含一个元素的插件上。

filter是处理数据流的重要元素类型。数据的生产者和消费者分别被称为source元素和sink元素。Bin元素包含其他元素。一种类型的Bin负责同步其包含的元素，以便数据流畅地传输。另一种类型的Bin称为自动加载元素，它会自动向Bin中添加其他元素并将它们链接在一起，以使它们充当两种任意流类型之间的过滤器。

插件机制在GStreamer中无处不在，即使只使用标准包。核心库中有一些非常基本的功能，所有其他功能都在插件中实现。插件注册表用于存储二进制注册表文件中的插件的详细信息。这样，使用GStreamer的程序不必加载所有插件以确定哪些是必需的。插件仅在请求其提供的元素时才加载。

请查看GStreamer库参考手册，了解[GstElement](https://gstreamer.freedesktop.org/documentation/gstreamer/gstelement.html?gi-language=c#GstElement)和[GstPlugin](https://gstreamer.freedesktop.org/documentation/gstreamer/gstplugin.html?gi-language=c#GstPlugin)的当前实现详细信息。

## 2 Pads

在GStreamer中，Pads用于协商元素之间的链接和数据流。一个Pad可以看作是元素上的一个“位置”或“端口”，可以在该位置与其他元素建立链接，并通过该位置进行数据的流入或流出。Pads具有特定的数据处理能力：一个Pad可以限制通过它流动的数据类型。只有当两个Pad的允许数据类型是兼容的时，才允许建立链接。

这里可以使用一个类比来帮助理解。一个Pad类似于物理设备上的插头或插孔。例如，考虑一个包括放大器、DVD播放机和（静音的）视频投影仪的家庭影院系统。允许将DVD播放机与放大器链接，因为两个设备都有音频插孔，允许将视频投影仪与DVD播放机链接，因为两个设备都有兼容的视频插孔。不允许在视频投影仪和放大器之间建立链接，因为它们具有不同类型的插孔。在GStreamer中，Pads的作用类似于家庭影院系统中的插孔。

在大多数情况下，GStreamer中的所有数据通过元素之间的链接单向流动。数据从一个元素流出，通过一个或多个源Pad，元素通过一个或多个汇Pad接收传入的数据。源元素和汇元素分别只有源Pad和汇Pad。

请查看GStreamer库参考手册，了解[GstPad](https://gstreamer.freedesktop.org/documentation/gstreamer/gstpad.html?gi-language=c#GstPad)的当前实现详细信息。

## 3 GstMiniObject, Buffers and Events

在GStreamer中，所有数据流都被分割成一小块一小块的数据，这些数据块从一个元素的源Pad传递到另一个元素的汇Pad。GstMiniObject是用于保存这些数据块的结构。

GstMiniObject包含以下重要的类型：

- 精确的类型，指示GstMiniObject是什么类型的数据（事件event、缓冲区buffer等）。

- 引用计数，指示当前持有对MiniObject引用的元素数量。当引用计数降至零时，MiniObject将被销毁，并在某种程度上释放其内存（有关更多详细信息，请参见下文）。

对于数据传输，定义了两种类型的GstMiniObject：事件（控制）和缓冲区（内容）。

缓冲区可以包含两个链接的Pad知道如何处理的任何类型的数据。通常，缓冲区包含从一个元素流向另一个元素的某种音频或视频数据块。

缓冲区还包含描述缓冲区内容的元数据。一些重要的元数据类型包括：

- 指向一个或多个GstMemory对象的指针。GstMemory对象是封装内存区域的引用计数对象。

- 时间戳，指示缓冲区中内容的首选显示时间戳。

事件包含有关两个链接的Pad之间流动状态的信息。仅当元素显式支持事件时，事件才会被发送，否则核心会（尝试）自动处理事件。事件用于指示媒体类型、媒体流的结束或应清空缓存等情况。

事件可能包含以下一些项目：

- 子类型，指示所包含事件的类型。

- 事件类型特定的其他内容。

事件将在“[事件：搜索、导航和更多](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/events.html?gi-language=c)”中广泛讨论。在那之前，将仅使用EOS事件，它用于指示流的结束（通常是文件的结束）。

请查看GStreamer库参考手册，了解[GstMiniObject](https://gstreamer.freedesktop.org/documentation/gstreamer/gstminiobject.html?gi-language=c#GstMiniObject)、[GstBuffer](https://gstreamer.freedesktop.org/documentation/gstreamer/gstbuffer.html?gi-language=c#GstBuffer)和[GstEvent](https://gstreamer.freedesktop.org/documentation/gstreamer/gstevent.html?gi-language=c#GstEvent)的当前实现详细信息。

## 4 Buffer Allocation

缓冲区能够存储多种不同类型的内存块。最通用的缓冲区类型包含由malloc()分配的内存。虽然这些缓冲区非常方便，但并不总是非常快，因为数据通常需要专门复制到缓冲区中。

许多专用的元素创建指向特殊内存的缓冲区。例如，filesrc元素通常将文件映射到应用程序的地址空间（使用mmap()），并创建指向该地址范围的缓冲区。这些由filesrc创建的缓冲区与通用缓冲区的工作方式完全相同，只是它们是只读的。缓冲区释放代码会自动确定释放底层内存的正确方法。接收这些类型的缓冲区的下游元素不需要采取任何特殊操作来处理或取消引用它。

元素可能获得专用缓冲区的另一种方式是通过GstBufferPool或GstAllocator从下游对等体请求。元素可以向下游对等体请求GstBufferPool或GstAllocator。如果下游能够提供这些对象，上游可以使用它们来分配缓冲区。有关更多信息，请参见[内存分配](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/allocation.html?gi-language=c)。

许多汇元素具有将数据快速复制到硬件或直接访问硬件的方法。这些元素通常能够为它们的上游对等体创建GstBufferPool或GstAllocator。一个典型的例子是ximagesink。它创建包含XImages的缓冲区。因此，当上游对等体将数据复制到缓冲区时，它直接复制到XImage中，从而使ximagesink能够直接将图像绘制到屏幕上，而不必先复制数据到XImage中。

过滤元素通常有机会在原地处理缓冲区，或者在从源缓冲区到目标缓冲区进行复制时进行处理。实现两种算法是最优的，因为GStreamer框架可以根据需要选择最快的算法。当然，这只对于严格的过滤器有意义，即在源Pad和汇Pad上具有完全相同格式的元素。

## 5 Media types and Properties

GStreamer使用一个类型系统来确保在元素之间传递的数据是在一个已识别的格式中。类型系统还对确保在元素之间链接Pad时正确匹配用于完全指定格式的参数非常重要。每个在元素之间创建的链接都具有指定的类型，以及可选的一组属性。有关有关能力协商的更多信息，请参见有关[能力协商](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/negotiation.html?gi-language=c)的文章。

## 6 The Basic Types

GStreamer已经支持许多基本的媒体类型。以下是GStreamer中用于缓冲区的一些基本类型的表格。该表格包含了类型的名称（"媒体类型"），类型相关的属性以及每个属性的含义。支持的所有类型的完整列表包含在"已定义类型列表"中。

