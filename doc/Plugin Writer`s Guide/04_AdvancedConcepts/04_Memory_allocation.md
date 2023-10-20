# 4 Memory allocation

内存分配和管理是多媒体领域中非常重要的主题。高清晰度视频需要大量兆字节来存储单个图像帧。在可能的情况下，重复使用内存非常重要，而不是不断地分配和释放内存。

多媒体系统通常使用专用芯片，如DSP或GPU来执行繁重的工作（尤其是视频）。这些专用芯片通常对它们操作的内存和访问方式有严格的要求。

本章讨论了GStreamer插件可用的内存管理功能。首先，我们将讨论低级别的GstMemory对象，它管理对一块内存的访问，然后继续讨论它的主要用户之一，GstBuffer，它用于在元素之间以及与应用程序之间交换数据。我们还将讨论GstMeta。这个对象可以放置在缓冲区上，以提供有关它们及其内存的额外信息。我们还将讨论GstBufferPool，它允许更高效地管理相同大小的缓冲区。

最后，我们将看一看GST_QUERY_ALLOCATION查询，该查询用于在元素之间协商内存管理选项。

## 1 GstMemory

### 1.1 GstAllocator

### 1.2 GstMemory API example

### 1.3 Implementing a GstAllocator

未写

## 2 GstBuffer

## 3 GstMeta

## 4 GstBufferPool

## 5 GST_QUERY_ALLOCATION

## 参考
[翻译自：Memory allocation](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/allocation.html?gi-language=c)