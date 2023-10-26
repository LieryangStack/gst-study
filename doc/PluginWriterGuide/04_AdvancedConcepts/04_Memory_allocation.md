# 4 Memory allocation

内存分配和管理是多媒体领域中非常重要的主题。高清晰度视频需要大量兆字节来存储单个图像帧。在可能的情况下，重复使用内存非常重要，而不是不断地分配和释放内存。

多媒体系统通常使用专用芯片，如DSP或GPU来执行繁重的工作（尤其是视频）。这些专用芯片通常对它们操作的内存和访问方式有严格的要求。

本章讨论了GStreamer插件可用的内存管理功能。首先，我们将讨论低级别的GstMemory对象，它管理对一块内存的访问，然后继续讨论它的主要用户之一，GstBuffer，它用于在元素之间以及与应用程序之间交换数据。我们还将讨论GstMeta。这个对象可以放置在缓冲区上，以提供有关它们及其内存的额外信息。我们还将讨论GstBufferPool，它允许更高效地管理相同大小的缓冲区。

最后，我们将看一看GST_QUERY_ALLOCATION查询，该查询用于在元素之间协商内存管理选项。

## 1 GstMemory

GstMemory是一个管理内存区域的对象。该内存对象指向一个"maxsize"大小的内存区域。从"offset"开始，大小为"size"字节的区域是可访问的内存区域。创建了GstMemory之后，它的"maxsize"不能再更改，但是它的"offset"和"size"可以更改。

```c
struct _GstMemory {
  GstMiniObject   mini_object;

  GstAllocator   *allocator;

  GstMemory      *parent;
  gsize           maxsize;
  gsize           align;
  gsize           offset;
  gsize           size;
};
```


<span style="background-color: pink; padding: 2px;">GstMemory轻量级对象，基于GstMiniObject，GstMemory对象由GstAllocator的gst_allocator_alloc()方法创建。</span>


### 1.1 GstAllocator

GstMemory对象是由GstAllocator对象创建的。大多数分配器实现了默认的gst_allocator_alloc()方法，但有些可能会实现不同的方法，例如，当需要额外的参数来分配特定的内存时。

不同的分配器用于系统内存、共享内存和由DMAbuf文件描述符支持的内存。要实现对新类型内存的支持，您必须实现一个新的分配器对象。

#### GstAllocatorClass类结构体

```c
/**
 * GstAllocatorClass:
 * @object_class:  Object parent class
 * @alloc: 申请内存
 * @free: 释放内存
 *
 * The #GstAllocator is used to create new memory.
 */
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

#### GstAllocator实例结构体

```c
/**
 * GstAllocator:
 * @mem_map: the implementation of the GstMemoryMapFunction
 * @mem_unmap: the implementation of the GstMemoryUnmapFunction
 * @mem_copy: the implementation of the GstMemoryCopyFunction
 * @mem_share: the implementation of the GstMemoryShareFunction
 * @mem_is_span: the implementation of the GstMemoryIsSpanFunction
 * @mem_map_full: the implementation of the GstMemoryMapFullFunction.
 *      Will be used instead of @mem_map if present. (Since: 1.6)
 * @mem_unmap_full: the implementation of the GstMemoryUnmapFullFunction.
 *      Will be used instead of @mem_unmap if present. (Since: 1.6)
 *
 * The #GstAllocator is used to create new memory.
 */
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
```


### 1.2 GstMemory API example

GstMemory对象封装的内存的数据访问始终需要使用gst_memory_map()和gst_memory_unmap()成对来进行保护。在映射内存时必须提供访问模式（读/写）。映射函数将返回指向有效内存区域的指针，然后可以根据请求的访问模式来访问它。

以下是一个创建GstMemory对象并使用gst_memory_map()来访问内存区域的示例：

```c
[...]

  GstMemory *mem;
  GstMapInfo info;
  gint i;

  /* allocate 100 bytes */
  mem = gst_allocator_alloc (NULL, 100, NULL);

  /* 其实就是计算实际内存地址，返回用户实际数据地址给info.data */
  gst_memory_map (mem, &info, GST_MAP_WRITE);

  /* fill with pattern */
  for (i = 0; i < info.size; i++)
    info.data[i] = i;

  /* release memory */
  gst_memory_unmap (mem, &info);

[...]
```

### 1.3 Implementing a GstAllocator

我们以系统GstAllocatorSystem为例，源代码在`gstreamer-1.22.6/subprojects/gstreamer/gst/gstallocator.c`

实现一个GstAllocator有以下两部分：

1. 基于GstMemory结构体，创建一个自定义的GstMemorySystem结构体

    ```c
    typedef struct
    {
      GstMemory mem;

      gsize slice_size;
      guint8 *data;

      gpointer user_data;
      GDestroyNotify notify;
    } GstMemorySystem;
    ```
2. 基于GstAllocator，实现一个新的GstAllocatorSystem（由于新的分配器对象实例和类一般不需要新的成员变量，通常是为了实现虚函数方法）
    ```c
    [...]

    typedef struct
    {
      GstAllocator parent;
    } GstAllocatorSysmem;

    typedef struct
    {
      GstAllocatorClass parent_class;
    } GstAllocatorSysmemClass;

    [...]

    static void
    gst_allocator_sysmem_class_init (GstAllocatorSysmemClass * klass)
    {
      GObjectClass *gobject_class;
      GstAllocatorClass *allocator_class;

      gobject_class = (GObjectClass *) klass;
      allocator_class = (GstAllocatorClass *) klass;

      gobject_class->finalize = gst_allocator_sysmem_finalize;
      /* 创建GstMemory调用该方法 */
      allocator_class->alloc = default_alloc;
      /* 释放GstMemory调用该方法 */
      allocator_class->free = default_free;
    }

    static void
    gst_allocator_sysmem_init (GstAllocatorSysmem * allocator)
    {
      GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

      GST_CAT_DEBUG (GST_CAT_MEMORY, "init allocator %p", allocator);

      alloc->mem_type = GST_ALLOCATOR_SYSMEM;
      alloc->mem_map = (GstMemoryMapFunction) _sysmem_map;
      alloc->mem_unmap = (GstMemoryUnmapFunction) _sysmem_unmap;
      alloc->mem_copy = (GstMemoryCopyFunction) _sysmem_copy;
      alloc->mem_share = (GstMemoryShareFunction) _sysmem_share;
      alloc->mem_is_span = (GstMemoryIsSpanFunction) _sysmem_is_span;
    }

    [...]
    ```
#### GstAllocator相关函数

这些函数供外部调用，从而实现创建内存和释放内存。

- **gst_allocator_alloc**

  ```c
  GstMemory *
  gst_allocator_alloc (GstAllocator * allocator, gsize size,
      GstAllocationParams * params)
  {
    GstMemory *mem;
    static GstAllocationParams defparams = { 0, 0, 0, 0, };
    GstAllocatorClass *aclass;

    if (params) {
      g_return_val_if_fail (((params->align + 1) & params->align) == 0, NULL);
    } else {
      params = &defparams;
    }

    if (allocator == NULL)
      /* 使用默认的分配器，既GstMemorySystem
       * _default_allocator系统初始化的时候创建
       * 实际创建的内存大小 = prefix + padding + align + 用户请求内存 + sizeof (GstMemorySystem)
       */
      allocator = _default_allocator;

    aclass = GST_ALLOCATOR_GET_CLASS (allocator);
    if (aclass->alloc)
      /* 调用 GstAllocatorClass->alloc虚函数分配内存*/
      mem = aclass->alloc (allocator, size, params);
    else
      mem = NULL;

    return mem;
  }
  ```

  分配内存的时候，用到了GstAllocationParams结构体，该结构体成员具体起什么作用呢？

  ```c
  struct _GstAllocationParams {
    GstMemoryFlags flags; /* 申请的内存是否可读写，共享等特性 */
    gsize          align; /* 内存对齐 */
    gsize          prefix; /* 预先需要多少内存 */
    gsize          padding; /* 尾部空余多少内存 */

    /*< private >*/
    gpointer _gst_reserved[GST_PADDING];
  };
  ```




- **gst_allocator_free**

  ```c
  void
  gst_allocator_free (GstAllocator * allocator, GstMemory * memory)
  {
    GstAllocatorClass *aclass;

    g_return_if_fail (GST_IS_ALLOCATOR (allocator));
    g_return_if_fail (memory != NULL);
    g_return_if_fail (memory->allocator == allocator);

    aclass = GST_ALLOCATOR_GET_CLASS (allocator);
    if (aclass->free)
      /* 调用 GstAllocatorClass->free虚函数释放内存*/
      aclass->free (allocator, memory);
  }
  ```

## 2 GstBuffer

GstBuffer是一个轻量级对象，从上游传递到下游元素，包含内存和元数据。它代表了被推送到或被下游元素拉取的多媒体内容。

<span style="background-color: pink;">GstBuffer包含一个或多个GstMemory对象，</span>这些对象保存了缓冲区的数据。

缓冲区中的元数据包括：

- DTS和PTS时间戳。这些时间戳表示缓冲区内容的解码和呈现时间戳，用于同步元素以安排缓冲区。当时间戳未知或未定义时，这些时间戳可以为GST_CLOCK_TIME_NONE。

- 缓冲区内容的持续时间。当持续时间未知或未定义时，这个持续时间可以为GST_CLOCK_TIME_NONE。

- 媒体特定的偏移offset和offset_end值。对于视频，这是流中的帧号，对于音频，这是样本号。其他媒体可能使用不同的定义。

- 通过GstMeta支持的任意结构，详情请见下文。


```c
struct _GstBuffer {
  GstMiniObject          mini_object;

  /*< public >*/ /* with COW */
  GstBufferPool         *pool;

  /* timestamp */
  GstClockTime           pts; /* 呈现时间戳 */
  GstClockTime           dts; /* 解码时间戳 */
  GstClockTime           duration; /* 缓冲区内容持续时间 */

  /* media specific offset */
  guint64                offset;
  guint64                offset_end;
};
```

### 2.1 Writability

当GstBuffer对象的引用计数（refcount）恰好为1时，GstBuffer是可写的，这意味着只有一个对象持有对缓冲区的引用。只有在缓冲区可写时，您才能修改它。这意味着在更改时间戳、偏移、元数据或添加和删除内存块之前，您需要调用gst_buffer_make_writable()。

### 2.2 API examples

您可以使用gst_buffer_new()创建一个GstBuffer，然后可以向其添加内存对象。或者，您可以使用便捷函数gst_buffer_new_allocate()一次性执行这两个操作。还可以使用gst_buffer_new_wrapped_full()来封装现有的内存，并指定在何时应释放内存的函数。

要访问GstBuffer的内存，可以通过逐个获取和映射GstMemory对象，也可以使用gst_buffer_map()。后者将所有内存合并为一个大块，然后提供一个指向它的指针。

以下是如何创建一个缓冲区并访问其内存的示例：

```c
[...]
  GstBuffer *buffer;
  GstMemory *mem;
  GstMapInfo info;

  /* make empty buffer */
  buffer = gst_buffer_new ();

  /* make memory holding 100 bytes */
  mem = gst_allocator_alloc (NULL, 100, NULL);

  /* add the buffer */
  gst_buffer_append_memory (buffer, mem);

[...]

  /* get WRITE access to the memory and fill with 0xff */
  gst_buffer_map (buffer, &info, GST_MAP_WRITE);
  memset (info.data, 0xff, info.size);
  gst_buffer_unmap (buffer, &info);

[...]

  /* free the buffer */
  gst_buffer_unref (buffer);

[...]
```

## 3 GstMeta

使用GstMeta系统，您可以向缓冲区添加任意结构。这些结构描述了缓冲区的附加属性，例如裁剪cropping、步幅stride、感兴趣区域region of interest等。

元数据系统将API规范（元数据及其API的外观）与其实现（工作原理）分开。这使得可以具有相同API的不同实现，例如，根据您运行的硬件的不同而有不同的实现。

### 3.1 API example

在分配了一个新的GstBuffer之后，您可以使用特定于元数据的API向其添加元数据。这意味着您需要链接到定义元数据的头文件，以使用其API。

按照惯例，名为FooBar的元数据API应该提供两个方法，gst_buffer_add_foo_bar_meta()和gst_buffer_get_foo_bar_meta()。这两个函数应返回指向包含元数据字段的FooBarMeta结构的指针。某些_add_*_meta()方法可能具有额外的参数，通常用于为您配置元数据结构。

让我们来看看用于指定视频帧裁剪区域的元数据。

```c
#include <gst/video/gstvideometa.h>

[...]
  GstVideoCropMeta *meta;

  /* buffer points to a video frame, add some cropping metadata */
  meta = gst_buffer_add_video_crop_meta (buffer);

  /* configure the cropping metadata */
  meta->x = 8;
  meta->y = 8;
  meta->width = 120;
  meta->height = 80;
[...]
```
然后，元素可以在呈现帧时使用缓冲区上的元数据，如下所示：
```c
#include <gst/video/gstvideometa.h>

[...]
  GstVideoCropMeta *meta;

  /* buffer points to a video frame, get the cropping metadata */
  meta = gst_buffer_get_video_crop_meta (buffer);

  if (meta) {
    /* render frame with cropping */
    _render_frame_cropped (buffer, meta->x, meta->y, meta->width, meta->height);
  } else {
    /* render frame */
    _render_frame (buffer);
  }
[...]
```

### 3.2 Implementing new GstMeta

在接下来的章节中，我们将展示如何向系统中添加新元数据，并在缓冲区上使用它。

**Define the metadata API**

首先，我们需要定义我们的API应该如何，然后注册这个API到系统中。这很重要，因为API的定义将在元素协商它们将交换什么类型的元数据时使用。API定义还包含了有关元数据包含的内容的任意标签。当我们查看元数据如何在缓冲区通过管线时保留时，这一点非常重要。

如果您正在制作一个现有API的新实现，可以跳过这一步，直接进行实现。

首先，我们从创建一个名为my-example-meta.h的头文件开始，该头文件将包含我们元数据的API定义和结构。

```c
#include <gst/gst.h>

typedef struct _MyExampleMeta MyExampleMeta;

struct _MyExampleMeta {
  GstMeta       meta;

  gint          age;
  gchar        *name;
};

GType my_example_meta_api_get_type (void);
#define MY_EXAMPLE_META_API_TYPE (my_example_meta_api_get_type())

#define gst_buffer_get_my_example_meta(b) \
  ((MyExampleMeta*)gst_buffer_get_meta((b),MY_EXAMPLE_META_API_TYPE))
```

元数据API的定义包括一个包含gint和string的结构的定义。结构中的第一个字段必须是GstMeta。

我们还定义了一个my_example_meta_api_get_type()函数，用于注册我们的元数据API定义，以及一个便捷的gst_buffer_get_my_example_meta()宏，它只是查找并返回具有我们新API的元数据。

让我们看看my_example_meta_api_get_type()函数在my-example-meta.c文件中是如何实现的：

```c
#include "my-example-meta.h"

GType
my_example_meta_api_get_type (void)
{
  static GType type;
  static const gchar *tags[] = { "foo", "bar", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("MyExampleMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}
```

如您所见，它简单地使用gst_meta_api_type_register()函数来注册API的名称和一些标签。结果是一个新的GType指针，定义了新注册的API。

接下来，我们可以为已注册的元数据API GType创建一个实现。

元数据API的实现细节保存在一个GstMetaInfo结构中，您可以使用my_example_meta_get_info()函数和便捷的MY_EXAMPLE_META_INFO宏将该结构提供给元数据API实现的用户。您还需要提供一个方法，将您的元数据实现添加到GstBuffer中。您的my-example-meta.h头文件将需要这些添加：

```c
[...]

/* implementation */
const GstMetaInfo *my_example_meta_get_info (void);
#define MY_EXAMPLE_META_INFO (my_example_meta_get_info())

MyExampleMeta * gst_buffer_add_my_example_meta (GstBuffer      *buffer,
                                                gint            age,
                                                const gchar    *name);
```
让我们看看这些函数在my-example-meta.c文件中是如何实现的。

```c
[...]

static gboolean
my_example_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  MyExampleMeta *emeta = (MyExampleMeta *) meta;

  emeta->age = 0;
  emeta->name = NULL;

  return TRUE;
}

static gboolean
my_example_meta_transform (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  MyExampleMeta *emeta = (MyExampleMeta *) meta;

  /* we always copy no matter what transform */
  gst_buffer_add_my_example_meta (transbuf, emeta->age, emeta->name);

  return TRUE;
}

static void
my_example_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  MyExampleMeta *emeta = (MyExampleMeta *) meta;

  g_free (emeta->name);
  emeta->name = NULL;
}

const GstMetaInfo *
my_example_meta_get_info (void)
{
  static const GstMetaInfo *meta_info = NULL;

  if (g_once_init_enter (&meta_info)) {
    const GstMetaInfo *mi = gst_meta_register (MY_EXAMPLE_META_API_TYPE,
        "MyExampleMeta",
        sizeof (MyExampleMeta),
        my_example_meta_init,
        my_example_meta_free,
        my_example_meta_transform);
    g_once_init_leave (&meta_info, mi);
  }
  return meta_info;
}

MyExampleMeta *
gst_buffer_add_my_example_meta (GstBuffer   *buffer,
                                gint         age,
                                const gchar *name)
{
  MyExampleMeta *meta;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  meta = (MyExampleMeta *) gst_buffer_add_meta (buffer,
      MY_EXAMPLE_META_INFO, NULL);

  meta->age = age;
  meta->name = g_strdup (name);

  return meta;
}
```

gst_meta_register()用于注册实现细节，例如您要实现的API以及元数据结构的大小，以及用于初始化和释放内存区域的方法。您还可以实现一个转换函数，当在缓冲区上执行某个特定的转换（由quark和quark特定数据标识）时，将调用该函数。

最后，您需要实现一个gst_buffer_add_*_meta()函数，它将元数据实现添加到缓冲区并设置元数据的值。

## 4 GstBufferPool

GstBufferPool对象提供了一个方便的基类，用于管理可重用缓冲区buffer的列表。此对象的关键点在于所有缓冲区都具有相同的属性，如大小、填充、元数据和对齐方式。

GstBufferPool可以配置为管理特定大小的缓冲区的最小和最大数量。它还可以配置为使用特定的GstAllocator来管理缓冲区的内存。缓冲池还支持启用特定的缓冲池选项，例如向缓冲池的缓冲区添加GstMeta或启用缓冲区内存的特定填充。

GstBufferPool可以处于非活动或活动状态。在非活动状态下，您可以配置缓冲池。在活动状态下，您不能再更改配置，但可以从缓冲池中获取和释放缓冲区。

在接下来的章节中，我们将介绍如何使用GstBufferPool。

### 4.1 API example

可以有许多不同的GstBufferPool实现；它们都是GstBufferPool基类的子类。在本例中，我们假设我们以某种方式可以访问缓冲池，要么是因为我们自己创建了它，要么是因为我们在下面将看到的ALLOCATION查询的结果中得到了一个。

GstBufferPool最初处于非活动状态，以便我们可以对其进行配置。尝试配置不在非活动状态的GstBufferPool将失败。同样，尝试激活未配置的缓冲池也将失败。

```c
  GstStructure *config;

[...]

  /* get config structure */
  config = gst_buffer_pool_get_config (pool);

  /* set caps, size, minimum and maximum buffers in the pool */
  gst_buffer_pool_config_set_params (config, caps, size, min, max);

  /* configure allocator and parameters */
  gst_buffer_pool_config_set_allocator (config, allocator, &params);

  /* store the updated configuration again */
  gst_buffer_pool_set_config (pool, config);

[...]

```

GstBufferPool的配置保存在一个通用的GstStructure中，可以使用gst_buffer_pool_get_config()获得。存在便捷方法来获取和设置该结构中的配置选项。在更新结构后，可以使用gst_buffer_pool_set_config()将其重新设置为GstBufferPool的当前配置。

可以在GstBufferPool上配置以下选项：

- 要分配的缓冲区的caps。

- 缓冲区的大小。这是池中缓冲区的建议大小。池可能决定分配更大的缓冲区以添加填充。

- 池中缓冲区的最小和最大数量。当将最小设置为> 0时，缓冲池将预分配这些数量的缓冲区。当最大值不为0时，缓冲池将分配最多最大数量的缓冲区。

- 要使用的分配器和参数。一些缓冲池可能会忽略分配器并使用其内部分配器。

- 使用字符串标识的其他任意缓冲池选项。缓冲池会列出支持的选项，您可以使用gst_buffer_pool_get_options()查询是否支持某个选项，还可以使用gst_buffer_pool_config_add_option()将选项添加到配置结构中。这些选项用于启用池在缓冲区上设置元数据或添加填充等额外配置选项。

配置设置在缓冲池上后，可以使用gst_buffer_pool_set_active(pool, TRUE)来激活缓冲池。从那时起，您可以使用gst_buffer_pool_acquire_buffer()从池中检索缓冲区，如下所示：

```c
 [...]

  GstFlowReturn ret;
  GstBuffer *buffer;

  ret = gst_buffer_pool_acquire_buffer (pool, &buffer, NULL);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto pool_failed;

  [...]
```

检查获取函数的返回值非常重要，因为它有可能失败：当您的元素关闭时，它将停用缓冲池，然后所有对获取函数的调用都将返回GST_FLOW_FLUSHING。

从池中获取的所有缓冲区都将其pool成员设置为原始缓冲池。当缓冲区上的最后一个引用计数递减时，GStreamer将自动调用gst_buffer_pool_release_buffer()来将缓冲区释放回池中。您（或任何其他下游元素）不需要知道缓冲区是否来自池，只需将其解引用即可。

### 4.2 Implementing a new GstBufferPool

未写

## 5 GST_QUERY_ALLOCATION

ALLOCATION查询用于在元素之间协商GstMeta、GstBufferPool和GstAllocator。分配策略的协商总是由src pad发起并决定的，src pad在协商了格式之后，在决定推送缓冲区之前发起。虽然下游的sinkpad可以建议一个分配策略，但最终是src pad基于下游sink pad的建议来决定。

src pad将使用已协商的caps作为参数执行GST_QUERY_ALLOCATION。这是为了让下游元素知道正在处理的媒体类型。下游的sinkpad可以用以下结果回应分配查询：

- 一个包含可能的GstBufferPool建议的数组，其中包括建议的大小、最小和最大缓冲区数量。

- 一个包含GstAllocator对象的数组，以及建议的分配参数，如标志、前缀、对齐和填充。当缓冲池支持时，这些分配器也可以在缓冲池中进行配置。

- 一个包含支持的GstMeta实现的数组，以及元数据特定的参数。在将元数据放在缓冲区之前，上游元素知道下游支持什么类型的元数据是很重要的。

当GST_QUERY_ALLOCATION返回时，源pad将从可用的缓冲池、分配器和元数据中选择如何分配缓冲区。

### 5.1 ALLOCATION query example

以下是ALLOCATION查询的示例。

```c
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

  GstCaps *caps;
  GstQuery *query;
  GstStructure *structure;
  GstBufferPool *pool;
  GstStructure *config;
  guint size, min, max;

[...]

  /* find a pool for the negotiated caps now */
  query = gst_query_new_allocation (caps, TRUE);

  if (!gst_pad_peer_query (scope->srcpad, query)) {
    /* query failed, not a problem, we use the query defaults */
  }

  if (gst_query_get_n_allocation_pools (query) > 0) {
    /* we got configuration from our peer, parse them */
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
  } else {
    pool = NULL;
    size = 0;
    min = max = 0;
  }

  if (pool == NULL) {
    /* we did not get a pool, make one ourselves then */
    pool = gst_video_buffer_pool_new ();
  }

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_set_config (pool, config);

  /* and activate */
  gst_buffer_pool_set_active (pool, TRUE);

[...]
```

这个特定的实现将创建一个自定义的GstVideoBufferPool对象，专门用于分配视频缓冲区。您还可以启用该池，让它在从池中获取的缓冲区上放置GstVideoMeta元数据，如下所示：

```c
gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META)
```

### 5.2 The ALLOCATION query in base classes

在许多基类中，您将看到以下用于影响分配策略的虚拟方法：

- propose_allocation() 应该为上游元素建议分配参数。

- decide_allocation() 应该从下游收到的建议中决定分配参数。

实现这些方法的人应该通过更新池选项和分配选项来修改给定的GstQuery对象。

### 5.3 Negotiating the exact layout of video buffers

硬件元素可能对其输入缓冲区的布局有特定的约束，需要在其平面上添加垂直和/或水平填充。如果生产者能够创建满足这些要求的缓冲区，我们可以在开始生产缓冲区之前通过配置其驱动程序来确保零拷贝。

在这种设置中，通常会使用dmabuf来交换缓冲区以减少内存拷贝。生产者可以将其缓冲区导出到消费者（dmabuf导出），或者从消费者导入缓冲区（dmabuf导入）。

在本节中，我们将概述如何在导入和导出用例中，消费者可以通知生产者其期望的缓冲区布局。让我们考虑v4l2src（生产者）向v4l2h264enc（消费者）进行编码的缓冲区导入的情况。

 1. v4l2h264enc：查询硬件以获取其要求，并相应创建一个GstVideoAlignment。
 2. v4l2h264enc：在其缓冲池alloc_buffer的实现中，调用gst_buffer_add_video_meta_full()，然后在返回的meta上调用gst_video_meta_set_alignment()，使用请求的对齐方式。对齐方式将被添加到元数据中，允许v4l2src在尝试导入缓冲区之前配置其驱动程序。

 ```c
  meta = gst_buffer_add_video_meta_full (buf, GST_VIDEO_FRAME_FLAG_NONE,
          GST_VIDEO_INFO_FORMAT (&pool->video_info),
          GST_VIDEO_INFO_WIDTH (&pool->video_info),
          GST_VIDEO_INFO_HEIGHT (&pool->video_info),
          GST_VIDEO_INFO_N_PLANES (&pool->video_info), offset, stride);

      gst_video_meta_set_alignment (meta, align);
 ```

3. v4l2h264enc：在回应 ALLOCATION 查询（propose_allocation()）时，向生产者提议自己的内存池。

4. v4l2src：在收到 ALLOCATION 查询的回应（decide_allocation()）后，从建议的内存池中获取一个单一的缓冲区，并使用 GstVideoMeta.stride 和 gst_video_meta_get_plane_height() 检索其布局。

5. v4l2src：尽可能地配置其驱动程序以生成与这些要求匹配的数据，然后尝试导入缓冲区。如果无法导入，v4l2src 将无法从 v4l2h264enc 导入，因此将退回到将自己的缓冲区发送给 v4l2h264enc，后者将不得不复制每个输入缓冲区以符合其要求。

**v4l2src exporting buffers to v4l2h264enc**
1. v4l2h264enc: 查询硬件的需求并相应地创建一个 GstVideoAlignment。

2. v4l2h264enc: 创建一个名为 video-meta 的 GstStructure，将对齐信息串行化为其中。

```c
params = gst_structure_new ("video-meta",
    "padding-top", G_TYPE_UINT, align.padding_top,
    "padding-bottom", G_TYPE_UINT, align.padding_bottom,
    "padding-left", G_TYPE_UINT, align.padding_left,
    "padding-right", G_TYPE_UINT, align.padding_right,
    NULL);
```
3. v4l2h264enc: 在处理 ALLOCATION 查询（propose_allocation()）时，将此结构作为参数传递，用于添加 GST_VIDEO_META_API_TYPE 类型的元数据：

```c
gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, params);
```

4. v4l2src: 在收到 ALLOCATION 查询的回应（decide_allocation()）时，检索 GST_VIDEO_META_API_TYPE 类型的参数，以计算预期的缓冲区布局：

```c
guint video_idx;
GstStructure *params;

if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, &video_idx)) {
  gst_query_parse_nth_allocation_meta (query, video_idx, &params);

  if (params) {
    GstVideoAlignment align;
    GstVideoInfo info;
    gsize plane_size[GST_VIDEO_MAX_PLANES];

    gst_video_alignment_reset (&align);

    gst_structure_get_uint (s, "padding-top", &align.padding_top);
    gst_structure_get_uint (s, "padding-bottom", &align.padding_bottom);
    gst_structure_get_uint (s, "padding-left", &align.padding_left);
    gst_structure_get_uint (s, "padding-right", &align.padding_right);

    gst_video_info_from_caps (&info, caps);

    gst_video_info_align_full (&info, align, plane_size);
  }
}
```

5. v4l2src: 使用 GstVideoInfo.stride 和 GST_VIDEO_INFO_PLANE_HEIGHT() 检索请求的缓冲区布局。

6. v4l2src: 如果可能的话，配置其驱动程序以生成与这些要求匹配的数据。如果无法满足要求，驱动程序将生成具有自己布局的缓冲区，但 v4l2h264enc 将不得不复制每个输入缓冲区以符合其要求。

## 参考
[翻译自：Memory allocation](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/allocation.html?gi-language=c)