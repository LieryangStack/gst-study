## 目标
有时事情不会按照预期进行，从总线(如果有的话)检索到的错误消息不能提供足够的信息。幸运的是，GStreamer附带了大量调试信息，这些信息通常会提示问题可能是什么。本教程展示了:

 - 如何从GStreamer获取更多调试信息。

 - 如何将自己的调试信息打印到GStreamer日志中。

- 如何获得管道图（pipeline graphs）

## 打印调试信息
### 调试日志
GStreamer及其插件充满了调试跟踪，这是在代码中特别有趣的一块信息打印到控制台的地方，连同时间戳，进程，类别，源代码文件，函数和元素信息。

调试输出由`GST_DEBUG`环境变量控制。下面是一个GST_DEBUG=2的例子:

> 0:00:00.868050000  1592   09F62420 WARN                 filesrc gstfilesrc.c:1044:gst_file_src_start:<filesrc0> error: No such file "non-existing-file.webm"

如你所见，这是相当多的信息。事实上，GStreamer调试日志是如此冗长，当完全启用时，它会使应用程序无响应(由于控制台滚动)或填满兆字节的文本文件(当重定向到一个文件时)。因此，日志是分类的，很少需要一次性启用所有类别。

第一类是调试级别，它是一个数字，通过数字指定了预期输出的内容:

|#|Name |描述 |
|--|--|--|
|0| none  |没有debug信息输出 |
|1|EROOR|记录所有致命错误。这些错误不允许核心或元素执行请求的操作。如果编程处理了触发错误的条件，应用程序仍然可以恢复。|
|2| WARNING|记录所有警告。通常情况下，这些不是致命的，但用户可见的问题是会发生的。|
|3|FIXME|记录所有“fixme”消息。那些通常是已知不完整的代码路径被触发。它在大多数情况下可能正常工作，但在特定情况下可能会导致问题。|
|4|INFO|记录所有info消息。它们通常用于系统中只发生一次的事件，或者是重要且罕见的事件，需要记录到此级别。|
|5|DEBUG|记录所有debug信息。这些是一般的调试消息，用于在对象的生命周期中只发生`有限次`的事件;这些操作包括设置、卸载、修改参数等。|
|6|LOG|记录所有log信息。这些消息是在对象的生命周期中`反复`发生的事件的消息;这包括streaming和steady-state。例如，这用于记录发生在元素中每个缓冲区上的消息。 |
|7|TRACE|记录所有跟踪消息。这些都是经常发生的信息。例如，每次GstMiniObject(如GstBuffer或GstEvent)的引用计数被修改。|
|9|MEMDUMP|记录所有内存转储消息。这是最重要的日志记录，可能包括转储内存块的内容|

要启用调试输出，请将GST_DEBUG环境变量设置为所需的调试级别。低于此级别（比如设置2，1等级的内容也会显示）的所有级别也将显示(即，如果设置GST_DEBUG=2，则将得到错误和警告消息)。

此外，每个插件或GStreamer的一部分定义了自己的类别，因此您可以为每个单独的类别指定一个调试级别。例如，GST_DEBUG=2,audiotestsrc:6，将对audiotestsrc元素使用调试级别6，对所有其他元素使用2。

因此，GST_DEBUG环境变量是一个逗号分隔的category:level对列表，开头是一个可选的级别，表示所有类别的默认调试级别。

通配符` * `也是可用的。例如，GST_DEBUG=2,audio*:6将对以单词audio开头的所有类别使用Debug Level 6。GST_DEBUG=*:2等价于GST_DEBUG=2。

使用`gst-launch-1.0 --gst-debug-help`获取所有注册类别的列表。记住，每个插件都注册了自己的类别，所以当安装或删除插件时，这个列表可能会改变。

当GStreamer总线上发布的错误信息不能帮助您确定问题时，请使用GST_DEBUG。通常的做法是将输出日志重定向到一个文件，然后稍后对其进行检查，搜索特定的消息。

GStreamer允许自定义调试信息处理程序，但在使用默认处理程序时，调试输出中的每行内容如下所示:

> 0:00:00.868050000  1592   09F62420 WARN                 filesrc gstfilesrc.c:1044:gst_file_src_start:<filesrc0> error: No such file "non-existing-file.webm"

这是信息的格式:
|Example|Explained|
|--|--|
|0:00:00.868050000|时间戳格式为HH:MM:SS.ssssssss自程序开始以来。|
|1592|发出消息的进程ID。当您的问题涉及多个过程时非常有用。|
|09F62420|线程发出消息的ID。当你的问题涉及多个线程时很有用。|
|WARN|消息的Debug等级。|
|filesrc|消息的Debug类别|
|gstfilesrc.c:1044 |发出此消息的GStreamer源代码中的源文件和行。|
|gst_file_src_start|发出此消息的函数|
|`<filesrc0>`|发出消息的对象的名称。它可以是一个element，一个pad，或者其他。当有多个同类元素需要区分时很有用。使用name属性命名元素可以使调试输出更具可读性，但GStreamer默认为每个新元素分配一个唯一的名称。|
|error: No such file .... | 实际消息 |
## 增加你自己的调试信息
在与GStreamer交互的代码部分，使用GStreamer的调试功能会很有趣。通过这种方式，您将所有调试输出放在同一个文件中，并且不同消息之间的时间关系得到保留。

为此，可以使用`GST_ERROR()`、`GST_WARNING()`、`GST_INFO()`、`GST_LOG()`和`GST_DEBUG()`宏。它们接受与printf相同的参数，并且使用默认类别(default将在输出日志中显示为调试类别)。

要将类别更改为更有意义的内容，请在代码顶部添加以下两行:

```c
GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category
```
然后是在用gst_init()初始化GStreamer之后的代码:

```c
GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");
```
这注册了一个新的类别(在应用程序运行期间:它不存储在任何文件中)，并将其设置为代码的默认类别。请参阅GST_DEBUG_CATEGORY_INIT()的文档。
## Getting pipeline graphs
对于那些管道开始变得太大，并且无法跟踪element之间的连接情况，GStreamer具有输出图形文件的功能。这些是.dot文件，可以通过GraphViz等免费程序读取，它们描述了管道的拓扑结构，以及每个link中caps的协商情况。

当使用playbin或uridecodebin等一体化元素时，这也非常方便，因为它们实例化了其中的几个元素。使用.dot文件来了解它们在内部创建了什么管道(并在此过程中学习一些GStreamer)。

要获得.dot文件，只需将GST_DEBUG_DUMP_DOT_DIR环境变量设置为指向您想要放置文件的文件夹。gst-launch-1.0将在每次状态更改时创建一个.dot文件，因此您可以看到caps协商的演变。如果不设置GST_DEBUG_DUMP_DOT_DIR变量，则停止使用该功能。在你的应用程序中，你可以使用GST_DEBUG_BIN_TO_DOT_FILE()和GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS()宏生成.dot文件。

这里有一个playbin生成管道的例子。它非常复杂，因为playbin可以处理许多不同的情况:您的手动管道通常不需要这么长。如果你的手动管道开始变得非常大，可以考虑使用playbin。

### 补充（pipeline graphs）
### GST_DEBUG 示例
#### 1.1 gst-launch-1.0
>GST_DEBUG_NO_COLOR=1 GST_DEBUG_FILE=pipeline.log GST_DEBUG=5 gst-launch-1.0 audiotestsrc ! autoaudiosink
##### 1.2 应用程序
```c
#include <gst/gst.h>
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category

int main(int argc, char *argv[]) {
  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* gst初始化后初始 , 0日志字符表示无颜色输出， 1表示有颜色输出*/
  GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

  /* 通过以下函数，可以在自己的函数中添加debug调试信息 */
  GST_ERROR("My msg: %d", 0);
  GST_WARNING("My msg: %d", 1);
  GST_INFO("My msg: %d", 2);
  GST_DEBUG("My msg: %d", 3);

  return 0;
}
```
```bash
GST_DEBUG=5 ./a.out
```
### 2 gst-launch-1.0命令
#### 2.1 生成dot
```bash
## 1.安装dot
sudo apt-get install graphviz
## 2.添加到环境变量，文件夹路径可以任意改变
export GST_DEBUG_DUMP_DOT_DIR=/home/lieryang/Pictures
## 3.运行管道
gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux ! vp8dec ! videoconvert ! autovideosink
```
#### 2.2 使用dot生成png图片
管道结束后，您可以看到.dot生成的文件，并且“* PLAYING_PAUSED *”通常用于生成图表。
> ls
0.00.00.141932377-gst-launch.NULL_READY.dot
0.00.04.511918311-gst-launch.READY_PAUSED.dot
0.00.04.517318055-gst-launch.PAUSED_PLAYING.dot
**0.00.56.729201365-gst-launch.PLAYING_PAUSED.dot**
0.00.56.737842300-gst-launch.PAUSED_READY.dot

```bash
dot -Tpng 0.00.56.729201365-gst-launch.PLAYING_PAUSED.dot > 0.00.56.729201365-gst-launch.PLAYING_PAUSED.png
```
![请添加图片描述](https://img-blog.csdnimg.cn/8d6b2319304e4e86921d38590387b8ca.png)
 - 橘黄色：src element 和 src pad
 -  紫色：sink element 和 sink pad
 -  绿色：一般的element（除src element 和sink element外）
 
 
 #### 2.3 C语言编写的应用程序
 

```c
GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
//与上面宏的不同是，会在变量file_name前面加上时间戳
GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
```

> ls
> pipeline.dot
> 0.00.00.074939400-pipeline.dot  

运行应用程序时前缀加上GST_DEBUG_DUMP_DOT_DIR=/home/hiccup/gst_pipeline

```c
GST_DEBUG_DUMP_DOT_DIR=/home/hiccup/gst_pipeline ./application [参数1] [参数2] ...
```

当然，也可以通过g_setenv函数设置环境变量（`切记：设定环境变量一定要在gst_init()初始化函数前面调用`）。
```c
g_setenv("GST_DEBUG_DUMP_DOT_DIR", "../pipeline-graph/", TRUE);

```


### 脚本助手
有时你可以生成很多不同的点文件。如果你想将它们每个都转换为PNG图片，可以使用这个脚本。指定点文件所在的文件夹(DOT_FILES_DIR)，以及要放置生成的PNG文件的文件夹(PNG_FILES_DIR)。

```bash
DOT_FILES_DIR="./"
PNG_FILES_DIR="./"

DOT_FILES=`ls $DOT_FILES_DIR | grep dot`

for dot_file in $DOT_FILES
do
  png_file=`echo $dot_file | sed s/.dot/.png/`
  dot -Tpng $DOT_FILES_DIR/$dot_file > $PNG_FILES_DIR/$png_file
done

```


[参考1：How to generate a GStreamer pipeline diagram](https://developer.ridgerun.com/wiki/index.php/How_to_generate_a_GStreamer_pipeline_diagram)
[参考2：Gstreamer生成pipeline流图](https://www.cnblogs.com/hiccuplh/p/16160935.html)
[参考3：Function Macros：GST_DEBUG_BIN_TO_DOT_FILE](https://gstreamer.freedesktop.org/documentation/gstreamer/debugutils.html?gi-language=c#GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS)
[参考4：Gstreamer&DeepStream生成pipeline图](https://www.cnblogs.com/nanmi/p/14943279.html)
## 结论
本教程展示了:

 - 如何使用GST_DEBUG环境变量从GStreamer获取更多调试信息。
 - 如何使用GST_ERROR()宏及其相关函数将您自己的调试信息打印到GStreamer日志中。
 - 如何使用GST_DEBUG_DUMP_DOT_DIR环境变量获取管道图。

很高兴在这里见到你，希望很快见到你!

[参考1：Basic tutorial 11: Debugging tools](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html?gi-language=c#basic-tutorial-11-debugging-tools)
[参考2：How to generate a GStreamer pipeline diagram](https://developer.ridgerun.com/wiki/index.php/How_to_generate_a_GStreamer_pipeline_diagram)