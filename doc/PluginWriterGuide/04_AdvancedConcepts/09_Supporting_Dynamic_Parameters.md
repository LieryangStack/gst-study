# 9 Supporting Dynamic Parameters

警告，此部分描述的是0.10版本，已经过时。

有时候，对象属性不足以控制影响元素行为的参数。在这种情况下，您可以将这些参数标记为"可控"。有意识的应用程序可以使用控制器子系统随时间动态调整属性值。

## 1 Getting Started

控制器子系统包含在gstcontroller库中。您需要在您的元素源文件中包含以下头文件：

```c
...
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>
...
```

尽管gstcontroller库可能已链接到宿主应用程序中，但您应确保在您的plugin_init函数中进行初始化：

```c
  static gboolean
  plugin_init (GstPlugin *plugin)
  {
    ...
    /* initialize library */
    gst_controller_init (NULL, NULL);
    ...
  }
```

并不是所有的 GObject 参数都需要实时控制。因此，下一步是标记可控参数，可以在 _class_init 方法中设置 GObject 参数时使用特殊标志 GST_PARAM_CONTROLLABLE 进行标记。

```c
  g_object_class_install_property (gobject_class, PROP_FREQ,
      g_param_spec_double ("freq", "Frequency", "Frequency of test signal",
          0.0, 20000.0, 440.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
```

## 2 The Data Processing Loop


在上一节中，我们学习了如何将 GObject 参数标记为可控制。应用程序开发人员可以为这些参数排队参数更改。控制器子系统采用的方法是使插件负责拉取更改。这只需要一项操作：

```c
gst_object_sync_values(element,timestamp);
```

这个调用通过调整元素的 GObject 属性，使给定时间戳的所有参数更改生效。同步速率由元素决定。

### 2.1 The Data Processing Loop for Video Elements

对于视频处理元素，最好是为每一帧同步一次。这意味着你应该将前面部分中描述的 gst_object_sync_values() 调用添加到元素的数据处理函数中。

### 2.2 The Data Processing Loop for Audio Elements

对于音频处理元素，情况没有视频处理元素那么简单。问题在于音频具有更高的速率。对于 PAL 视频，例如，每秒会处理 25 帧完整帧，但对于标准音频，每秒会处理 44100 个样本。很少有必要同步可控参数这么频繁。最简单的解决方案是每次处理缓冲时只有一个同步调用。这使得控制速率取决于缓冲区大小。

需要特定控制速率的元素需要打破其数据处理循环，以便每 n 个样本进行一次同步。