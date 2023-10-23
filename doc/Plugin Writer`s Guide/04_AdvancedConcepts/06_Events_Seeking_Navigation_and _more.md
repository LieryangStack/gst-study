# 6 Events: Seeking, Navigation and More

有许多不同类型的事件，但它们只有两种传递方式：向下游或向上游。非常重要的是要了解这两种方法的工作原理，因为如果管道中的一个元素没有正确处理它们，整个管道的事件系统将会失效。我们将在这里尝试解释这些方法是如何工作的，以及元素应该如何实现它们。

## 1 Downstream events

下游事件通过接收端插孔的事件处理程序接收，该处理程序在创建插孔时使用 gst_pad_set_event_function() 进行设置。

下游事件可以以两种方式传递：它们可以是带内的（与缓冲流一起序列化）或带外的（通过管道瞬间传输，可能不在处理缓冲的流线程中，超前于正在处理或在管道中排队的缓冲）。最常见的下游事件（SEGMENT、CAPS、TAG、EOS）都与缓冲流一起序列化。

以下是一个典型的事件处理函数：

## 2 Upstream events

## 3 All Events Together

## 参考
[翻译自：Events: Seeking, Navigation and More](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/events.html?gi-language=c#events-seeking-navigation-and-more)