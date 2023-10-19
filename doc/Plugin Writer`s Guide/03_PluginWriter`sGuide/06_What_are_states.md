# 6 What are states?
状态描述元素实例是否已初始化，是否准备传输数据，以及当前是否正在处理数据。GStreamer中定义了四种状态。
 - GST_STATE_NULL
 - GST_STATE_READY （已经初始化）
 - GST_STATE_PAUSED （准备传输数据）
 - GST_STATE_PLAYING （正在处理数据）

从现在起，它们将被简称为“NULL”、“READY”、“PAUSED”和“PLAYING”。

 - GST_STATE_NULL是元素的默认状态。在这种状态下，它没有分配任何运行时资源，没有加载任何运行时库，显然不能处理数据。

 - GST_STATE_READY是元素可能处于的下一个状态。在就绪状态下，元素拥有分配的所有默认资源(运行时库、运行时内存)。然而，它还没有分配或定义任何特定于流的东西。当从NULL变为就绪状态(GST_STATE_CHANGE_NULL_TO_READY)时，元素应该分配所有非流特定的资源，并加载运行时可加载的库(如果有的话)。如果反过来(从READY到NULL, GST_STATE_CHANGE_READY_TO_NULL)，则元素应该卸载这些库并释放所有已分配的资源。硬件设备就是这样的资源。`请注意，文件通常是流，因此应该将其视为流特有的资源。因此，它们不应该在这种状态下分配`。

 - GST_STATE_PAUSED是元素准备接受和处理数据时的状态。对于大多数元素来说，这个状态等同于PLAYING。唯一的例外是sink元素。Sink元素只接受一个数据缓冲区，然后进行块处理。此时，管道已经` prerolled `并准备好立即渲染数据。

 - GST_STATE_PLAYING是元素可以处于的最高状态。对于大多数元素来说，这个状态与PAUSED状态完全相同，它们接受并处理事件和缓冲区中的数据。只有sink元素需要区分暂停和播放状态。在播放状态下，sink元素实际上渲染传入的数据，例如将音频输出到声卡或将视频图像渲染到图像sink。

# Managing filter state
如果可能的话，元素应该从一个新的基类(预先创建的基类)派生。对于不同类型的sources、sinks和filter/transformation元素，有现成的通用基类。除此之外，还存在用于音频和视频元素等的专用基类。

如果你使用基类，你将很少需要自己处理状态更改。你所要做的就是覆盖基类的start()和stop()虚函数(可能会根据基类的不同调用)，基类将为你处理一切。

但是，如果您不是从一个现成的基类派生而来，而是从GstElement或其他一些没有构建在基类之上的类派生而来，那么您很可能必须实现自己的状态更改函数以获得状态更改的通知。如果您的插件是一个demuxer或muxer，这是绝对必要的，因为还没有用于demuxer或muxer的基类。

可以通过虚函数指针通知元素状态的变化。在这个函数内部，元素可以初始化元素所需的任何特定类型的数据，并且可以选择从一种状态切换到另一种状态。

对于未处理的状态变化，不要使用g_assert。这是由GstElement基类处理的。

```c
static GstStateChangeReturn
gst_my_filter_change_state (GstElement *element, GstStateChange transition);

static void
gst_my_filter_class_init (GstMyFilterClass *klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  element_class->change_state = gst_my_filter_change_state;
}



static GstStateChangeReturn
gst_my_filter_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstMyFilter *filter = GST_MY_FILTER (element);

  switch (transition) {tr
	case GST_STATE_CHANGE_NULL_TO_READY:
	  if (!gst_my_filter_allocate_memory (filter))
		return GST_STATE_CHANGE_FAILURE;
	  break;
	default:
	  break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
	return ret;

  switch (transition) {
	case GST_STATE_CHANGE_READY_TO_NULL:
	  gst_my_filter_free_memory (filter);
	  break;
	default:
	  break;
  }

  return ret;
}

```
请注意，向上(NULL=>READY, READY=>PAUSED, PAUSED=>PLAYING)和向下(PLAYING =>PAUSED, PAUSED=>READY, READY=>NULL)状态更改在两个单独的块中处理，向下状态更改只有在我们`链接到父类的状态更改函数`后才处理。为了安全地处理多个线程的并发访问，这是必要的。

原因是，在向下状态变化的情况下，你不希望在插件的chain函数(例如)仍然在另一个线程中访问这些资源时销毁已分配的资源。链式函数是否在运行取决于插件的plugin's pads状态，而这些plugin's pads的状态与元素的状态密切相关。Pad状态是在GstElement类的状态改变函数中处理的，包括正确的锁定，这就是为什么在销毁分配的资源之前必须将其链接起来。