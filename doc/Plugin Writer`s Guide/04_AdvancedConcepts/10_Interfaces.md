# 10 Interfaces

在之前的“[添加属性](../03_PluginWriter`sGuide/07_Adding_Properties.md)”章节中，我们介绍了使用 GObject 属性来控制元素行为的概念。这非常强大，但有两个主要缺点：首先，它过于通用，其次，它不是动态的。

第一个缺点与最终用户界面的可定制性有关，该界面将用于控制元素。某些属性比其他属性更重要。一些整数属性更适合在自旋按钮小部件中显示，而其他属性更适合由滑块小部件表示。这种情况是不可能的，因为应用程序中的 UI 对 UI 小部件没有实际含义。表示比特率属性的 UI 小部件与表示视频大小的 UI 小部件相同，只要它们属于相同的 GParamSpec 类型。另一个问题是，参数分组、功能分组或参数耦合之类的事情实际上并不可行。

参数的第二个问题是它们不是动态的。在许多情况下，属性的允许值不是固定的，而是依赖于只能在运行时检测到的事物。例如，视频4linux源元素中的 TV 卡输入的名称只能在打开设备后才能从内核驱动程序中检索到；这只会在元素进入 READY 状态时发生。这意味着我们无法创建一个枚举属性类型来向用户显示这个信息。

解决这些问题的方法是为某些经常使用的控件创建非常专业化的类型。我们使用接口的概念来实现这一点。这一切的基础是 glib GTypeInterface 类型。对于我们认为有用的每种情况，我们都创建了可以由元素自行实现的接口。

一个重要的注意事项：接口并不取代属性。相反，接口应该与属性一起构建。这样做有两个重要原因。首先，属性更容易内省。其次，属性可以在命令行上指定（例如 gst-launch-1.0）。

## 1 How to Implement Interfaces

在元素的 _get_type() 函数中启用接口。在注册类型本身之后，您可以注册一个或多个接口。某些接口依赖于其他接口，或只能由某种类型的元素注册。如果您在注册支持您希望支持的接口之前注册了不正确的接口，您将在使用该元素时收到通知：它将以失败的断言退出，从而解释了发生了什么错误。如果发生这种情况，您需要在注册对您想要支持的接口的支持之前注册对该接口的支持。下面的示例说明了如何添加对没有进一步依赖的简单接口的支持。

```c
static void gst_my_filter_some_interface_init   (GstSomeInterface *iface);

GType
gst_my_filter_get_type (void)
{
  static GType my_filter_type = 0;

  if (!my_filter_type) {
    static const GTypeInfo my_filter_info = {
      sizeof (GstMyFilterClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_my_filter_class_init,
      NULL,
      NULL,
      sizeof (GstMyFilter),
      0,
      (GInstanceInitFunc) gst_my_filter_init
    };
    static const GInterfaceInfo some_interface_info = {
      (GInterfaceInitFunc) gst_my_filter_some_interface_init,
      NULL,
      NULL
    };

    my_filter_type =
    g_type_register_static (GST_TYPE_ELEMENT,
                "GstMyFilter",
                &my_filter_info, 0);
    g_type_add_interface_static (my_filter_type,
                 GST_TYPE_SOME_INTERFACE,
                                 &some_interface_info);
  }

  return my_filter_type;
}

static void
gst_my_filter_some_interface_init (GstSomeInterface *iface)
{
  /* here, you would set virtual function pointers in the interface */
}
```

或者更简便的写法：

```c
static void gst_my_filter_some_interface_init   (GstSomeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GstMyFilter, gst_my_filter,GST_TYPE_ELEMENT,
     G_IMPLEMENT_INTERFACE (GST_TYPE_SOME_INTERFACE,
            gst_my_filter_some_interface_init));

GST_ELEMENT_REGISTER_DEFINE(my_filter, "my-filter", GST_RANK_NONE, GST_TYPE_MY_FILTER);
```

## 2 URI interface

未编写

## 3 Color Balance Interface

未编写

## 4 Video Overlay Interface

GstVideoOverlay接口主要用于两个目的：

- 获取视频输出元素将要渲染的窗口。可以通过以下方式实现：要么获取视频输出元素生成的窗口标识符，要么强制视频输出元素使用特定的窗口标识符进行渲染。

- 强制重新绘制视频输出元素显示在窗口上的最新视频帧。事实上，如果GstPipeline处于GST_STATE_PAUSED状态，移动窗口会损坏其内容。应用程序开发人员将希望自己处理Expose事件，并强制视频输出元素刷新窗口的内容。

一个插件在视频窗口中绘制视频输出将在某个阶段需要该窗口。被动模式（passive mode）意味着在那个阶段之前未为插件提供窗口，因此插件自己创建了窗口。在这种情况下，插件负责在不再需要窗口时销毁该窗口，并且必须告知应用程序已经创建了一个窗口，以便应用程序可以使用它。这可以通过使用have-window-handle消息来实现，该消息可以由插件使用gst_video_overlay_got_window_handle方法发布。

正如您可能已经猜到的那样，主动模式（active mode）只是将视频窗口发送给插件，以便视频输出到该窗口。这可以通过使用gst_video_overlay_set_window_handle方法来实现。

可以在任何时刻从一种模式切换到另一种模式，因此实现此接口的插件必须处理所有情况。插件作者只需要实现两种方法，它们大致如下：

```c
static void
gst_my_filter_set_window_handle (GstVideoOverlay *overlay, guintptr handle)
{
  GstMyFilter *my_filter = GST_MY_FILTER (overlay);

  if (my_filter->window)
    gst_my_filter_destroy_window (my_filter->window);

  my_filter->window = handle;
}

static void
gst_my_filter_xoverlay_init (GstVideoOverlayClass *iface)
{
  iface->set_window_handle = gst_my_filter_set_window_handle;
}
```

您还需要使用接口方法在需要时发布消息，比如在接收到CAPS事件时，您将了解到视频的几何结构，也许需要创建窗口。

```c
static MyFilterWindow *
gst_my_filter_window_create (GstMyFilter *my_filter, gint width, gint height)
{
  MyFilterWindow *window = g_new (MyFilterWindow, 1);
  ...
  gst_video_overlay_got_window_handle (GST_VIDEO_OVERLAY (my_filter), window->win);
}

/* called from the event handler for CAPS events */
static gboolean
gst_my_filter_sink_set_caps (GstMyFilter *my_filter, GstCaps *caps)
{
  gint width, height;
  gboolean ret;
  ...
  ret = gst_structure_get_int (structure, "width", &width);
  ret &= gst_structure_get_int (structure, "height", &height);
  if (!ret) return FALSE;

  gst_video_overlay_prepare_window_handle (GST_VIDEO_OVERLAY (my_filter));

  if (!my_filter->window)
    my_filter->window = gst_my_filter_create_window (my_filter, width, height);

  ...
}
```

## 5 Navigation Interface

未编写