# 剖析ximagesink

XImageSink将视频帧呈现到本地或远程显示器上的可绘制区域（XWindow）。该元素可以通过[GstVideoOverlay](https://gstreamer.freedesktop.org/documentation/video/gstvideooverlay.html?gi-language=c#GstVideoOverlay)接口从应用程序接收窗口ID，然后将视频帧呈现在这个可绘制区域中。如果应用程序未提供窗口ID，该元素将创建自己的内部窗口并在其中进行渲染。

### Scaling

由于标准的XImage呈现不支持缩放，XImageSink将使用反向Caps协商尝试获取可绘制区域的缩放视频帧。这是通过询问对等源Pad是否接受一些不同的Caps来实现的，大多数情况下，这意味着在管道中存在一个缩放元素，或者生成视频帧的元素可以以不同的几何形状生成它们。这个机制在缓冲区分配期间处理，对于每个分配请求，视频渲染器将检查可绘制区域的几何形状，查看force-aspect-ratio属性，计算所需视频帧的几何形状，然后检查对等源Pad是否接受这些新的Caps。如果是，它将使用这个新的几何形状在视频内存中分配一个缓冲区，并使用新的Caps返回它。

### Event

XImageSink创建了一个线程来处理从可绘制区域传入的事件。这些事件可以分为两大类：输入事件和窗口状态相关事件。输入事件将被翻译为导航事件并推送到上游以供其他元素对其作出反应。这包括指针移动、按键按下/释放、点击等事件。其他事件用于处理可绘制区域的外观，即使数据不流动（GST_STATE_PAUSED）。这意味着即使元素被暂停，它仍然会接收来自可绘制区域的曝光事件，并绘制具有正确边框/纵横比的最新帧。

### Pixel aspect ratio

当将状态更改为GST_STATE_READY时，XImageSink将打开到显示的连接，该显示由display属性指定，如果没有指定，则使用默认显示。一旦建立了此连接，它将检查显示配置，包括物理显示几何形状，然后计算像素宽高比。在发生Caps协商时，视频渲染器将在Caps上设置计算出的像素宽高比，以确保传入的视频帧具有该显示的正确像素宽高比。有时，计算出的像素宽高比可能是错误的，此时可以使用pixel-aspect-ratio属性强制指定特定的像素宽高比。

### Examples

```bash
gst-launch-1.0 -v videotestsrc ! queue ! ximagesink
```
用于测试反向协商的管道。当测试视频信号出现时，您可以调整窗口大小，然后看到以所需大小的缓冲区稍有延迟地到达。这说明了如何在传输过程中分配所需大小的缓冲区。如果去掉队列，缩放几乎会立即发生。


```bash
 gst-launch-1.0 -v videotestsrc ! navigationtest ! videoconvert ! ximagesink
```

用于测试导航事件的管道。当将鼠标指针移动到测试信号上时，您将看到一个黑色框跟随鼠标指针移动。如果您在视频的某个地方按下鼠标按钮，然后在其他地方释放它，将会在按下按钮的位置出现一个绿色框，释放按钮的位置出现一个红色框。（navigationtest元素是gst-plugins-good的一部分。）

```bash
 gst-launch-1.0 -v videotestsrc ! "video/x-raw, pixel-aspect-ratio=(fraction)4/3" ! videoscale ! ximagesink
```

这是通过videotestsrc生成的视频帧上虚拟了一个4/3的像素宽高比Caps，在大多数情况下，显示的像素宽高比将为1/1。这意味着videoscale将不得不进行缩放，以将传入的帧转换为与显示像素宽高比匹配的大小（在此情况下从320x240到320x180）。请注意，您可能需要为您的shell转义一些字符，比如“(分数)”。

### 继承关系

```c
GObject
    ╰──GInitiallyUnowned
        ╰──GstObject
            ╰──GstElement
                ╰──GstBaseSink
                    ╰──GstVideoSink
                        ╰──ximagesink
```

### 实现接口

```
GstNavigation
GstVideoOverlay
```

[以上内容翻译自：gstreamer官方对于ximagesink介绍](https://gstreamer.freedesktop.org/documentation/ximagesink/index.html?gi-language=c)

## 1 

```bash
├── meson.build
├── ximage.c
├── ximagepool.c
├── ximagepool.h
├── ximagesink.c
└── ximagesink.h
```

生成libximagesink.so动态库的源文件目录`$HOME/gstreamer-1.22.6/subprojects/gst-plugins-base/sys/ximage`

### GstAllocator

内存通常是通过调用 gst_allocator_alloc 方法来由分配器创建的。当使用 NULL 作为分配器时，将使用默认的分配器。

可以使用 gst_allocator_register 注册新的分配器。分配器由名称标识，可以使用 gst_allocator_find 检索。可以使用 gst_allocator_set_default 来更改默认分配器。

可以使用 gst_memory_new_wrapped 创建新的内存，它包装了在其他地方分配的内存。

```c
GObject
    ╰──GInitiallyUnowned
        ╰──GstObject
            ╰──GstAllocator
```


```c
struct _GstAllocator
{
  GstObject  object;

  const gchar               *mem_type;

  /*< public >*/
  GstMemoryMapFunction       mem_map;
  GstMemoryUnmapFunction     mem_unmap;

  GstMemoryCopyFunction      mem_copy;
  GstMemoryShareFunction     mem_share;
  GstMemoryIsSpanFunction    mem_is_span;

  GstMemoryMapFullFunction   mem_map_full;
  GstMemoryUnmapFullFunction mem_unmap_full;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING - 2];

  GstAllocatorPrivate *priv;
};

struct _GstAllocatorClass {
  GstObjectClass object_class;

  /*< public >*/
  GstMemory *  (*alloc)      (GstAllocator *allocator, gsize size,
                              GstAllocationParams *params);
  void         (*free)       (GstAllocator *allocator, GstMemory *memory);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};
```


### GstBufferPool

GstBufferPool 是一个可用于预分配和回收相同大小和相同属性的缓冲区的对象。

可以使用 gst_buffer_pool_new 创建 GstBufferPool。

一旦创建了缓冲池，就需要对其进行配置。通过调用 gst_buffer_pool_get_config，可以从缓冲池中获取当前的配置结构。使用 gst_buffer_pool_config_set_params 和 gst_buffer_pool_config_set_allocator 可以配置缓冲池的参数和分配器。根据缓冲池的实现，还可以配置其他属性。

缓冲池可以具有可通过 gst_buffer_pool_config_add_option 启用的额外选项。可以使用 gst_buffer_pool_get_options 检索可用选项。某些选项允许设置额外的配置属性。

在配置结构已经配置后，使用 gst_buffer_pool_set_config 更新缓冲池中的配置。当配置结构不被接受时，这可能会失败。

在缓冲池配置完成后，可以使用 gst_buffer_pool_set_active 激活它。这将在缓冲池中预分配配置好的资源。

当缓冲池处于活动状态时，可以使用 gst_buffer_pool_acquire_buffer 从缓冲池中获取缓冲区。

从缓冲池分配的缓冲区在其引用计数降至0时将自动返回到缓冲池，使用 gst_buffer_pool_release_buffer。

可以再次使用 gst_buffer_pool_set_active 将缓冲池取消激活。此后的所有 gst_buffer_pool_acquire_buffer 调用将返回错误。当所有缓冲区都被返回到缓冲池时，它们将被释放。

```sh
GObject
    ╰──GInitiallyUnowned
        ╰──GstObject
            ╰──GstBufferPool
```

```c
/**
 * GstBufferPoolClass:
 * @object_class:  Object parent class
 *
 * The #GstBufferPool class.
 */
struct _GstBufferPoolClass {
  GstObjectClass    object_class;

  /*< public >*/

  /**
   * GstBufferPoolClass::get_options:
   * @pool: the #GstBufferPool
   *
   * Get a list of options supported by this pool
   *
   * Returns: (array zero-terminated=1) (transfer none): a %NULL terminated array
   *          of strings.
   */
  const gchar ** (*get_options)    (GstBufferPool *pool);

  /**
   * GstBufferPoolClass::set_config:
   * @pool: the #GstBufferPool
   * @config: the required configuration
   *
   * Apply the bufferpool configuration. The default configuration will parse
   * the default config parameters.
   *
   * Returns: whether the configuration could be set.
   */
  gboolean       (*set_config)     (GstBufferPool *pool, GstStructure *config);

  /**
   * GstBufferPoolClass::start:
   * @pool: the #GstBufferPool
   *
   * Start the bufferpool. The default implementation will preallocate
   * min-buffers buffers and put them in the queue.
   *
   * Returns: whether the pool could be started.
   */
  gboolean       (*start)          (GstBufferPool *pool);

  /**
   * GstBufferPoolClass::stop:
   * @pool: the #GstBufferPool
   *
   * Stop the bufferpool. the default implementation will free the
   * preallocated buffers. This function is called when all the buffers are
   * returned to the pool.
   *
   * Returns: whether the pool could be stopped.
   */
  gboolean       (*stop)           (GstBufferPool *pool);

  /**
   * GstBufferPoolClass::acquire_buffer:
   * @pool: the #GstBufferPool
   * @buffer: (out) (transfer full) (nullable): a location for a #GstBuffer
   * @params: (transfer none) (nullable): parameters.
   *
   * Get a new buffer from the pool. The default implementation
   * will take a buffer from the queue and optionally wait for a buffer to
   * be released when there are no buffers available.
   *
   * Returns: a #GstFlowReturn such as %GST_FLOW_FLUSHING when the pool is
   * inactive.
   */
  GstFlowReturn  (*acquire_buffer) (GstBufferPool *pool, GstBuffer **buffer,
                                    GstBufferPoolAcquireParams *params);

  /**
   * GstBufferPoolClass::alloc_buffer:
   * @pool: the #GstBufferPool
   * @buffer: (out) (transfer full) (nullable): a location for a #GstBuffer
   * @params: (transfer none) (nullable): parameters.
   *
   * Allocate a buffer. the default implementation allocates
   * buffers from the configured memory allocator and with the configured
   * parameters. All metadata that is present on the allocated buffer will
   * be marked as #GST_META_FLAG_POOLED and #GST_META_FLAG_LOCKED and will
   * not be removed from the buffer in #GstBufferPoolClass::reset_buffer.
   * The buffer should have the #GST_BUFFER_FLAG_TAG_MEMORY cleared.
   *
   * Returns: a #GstFlowReturn to indicate whether the allocation was
   * successful.
   */
  GstFlowReturn  (*alloc_buffer)   (GstBufferPool *pool, GstBuffer **buffer,
                                    GstBufferPoolAcquireParams *params);

  /**
   * GstBufferPoolClass::reset_buffer:
   * @pool: the #GstBufferPool
   * @buffer: the #GstBuffer to reset
   *
   * Reset the buffer to its state when it was freshly allocated.
   * The default implementation will clear the flags, timestamps and
   * will remove the metadata without the #GST_META_FLAG_POOLED flag (even
   * the metadata with #GST_META_FLAG_LOCKED). If the
   * #GST_BUFFER_FLAG_TAG_MEMORY was set, this function can also try to
   * restore the memory and clear the #GST_BUFFER_FLAG_TAG_MEMORY again.
   */
  void           (*reset_buffer)   (GstBufferPool *pool, GstBuffer *buffer);

  /**
   * GstBufferPoolClass::release_buffer:
   * @pool: the #GstBufferPool
   * @buffer: the #GstBuffer to release
   *
   * Release a buffer back in the pool. The default implementation
   * will put the buffer back in the queue and notify any
   * blocking #GstBufferPoolClass::acquire_buffer calls when the
   * #GST_BUFFER_FLAG_TAG_MEMORY is not set on the buffer.
   * If #GST_BUFFER_FLAG_TAG_MEMORY is set, the buffer will be freed with
   * #GstBufferPoolClass::free_buffer.
   */
  void           (*release_buffer) (GstBufferPool *pool, GstBuffer *buffer);

  /**
   * GstBufferPoolClass::free_buffer:
   * @pool: the #GstBufferPool
   * @buffer: the #GstBuffer to free
   *
   * Free a buffer. The default implementation unrefs the buffer.
   */
  void           (*free_buffer)    (GstBufferPool *pool, GstBuffer *buffer);

  /**
   * GstBufferPoolClass::flush_start:
   * @pool: the #GstBufferPool
   *
   * Enter the flushing state.
   *
   * Since: 1.4
   */
  void           (*flush_start)    (GstBufferPool *pool);

  /**
   * GstBufferPoolClass::flush_stop:
   * @pool: the #GstBufferPool
   *
   * Leave the flushing state.
   *
   * Since: 1.4
   */
  void           (*flush_stop)     (GstBufferPool *pool);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING - 2];
};
```

```c
/**
 * GstBufferPool:
 * @object: the parent structure
 * @flushing: whether the pool is currently gathering back outstanding buffers
 *
 * The structure of a #GstBufferPool. Use the associated macros to access the public
 * variables.
 */
struct _GstBufferPool {
  GstObject            object;

  /*< protected >*/
  gint                 flushing;

  /*< private >*/
  GstBufferPoolPrivate *priv;

  gpointer _gst_reserved[GST_PADDING];
};
```

https://blog.csdn.net/ykun089/article/details/121539374
https://blog.csdn.net/ykun089/category_12358954.html
https://blog.csdn.net/yuangc/article/details/122049058