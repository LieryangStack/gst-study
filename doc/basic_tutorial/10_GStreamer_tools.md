## 目标
GStreamer附带了一套工具，从方便到绝对必要。本教程中没有代码，只要坐下来放松一下，我们就会教你:

 - 如何从命令行构建和运行GStreamer管道，完全不使用C !
 - 如何找出可用的GStreamer元素及其功能capabilities。
 - 如何发现媒体文件的内部结构。
 
## 介绍
这些工具在GStreamer二进制文件的bin目录中。用户需要移动到该目录才能执行它们，因为它没有被添加到系统的PATH环境变量中(以避免过度污染它)。

只需打开一个终端(或控制台窗口)，并转到您的GStreamer安装的bin目录(再次阅读安装GStreamer部分以找到我们的位置)，然后您就可以开始输入入本教程中给出的命令。

为了允许在同一个系统中同时存在多个版本的GStreamer，这些工具是版本化的，即在它们的名称后面附加一个GStreamer版本号。这个版本基于GStreamer 1.0，因此这些工具被称为gst-launch-1.0、gst-inspect-1.0和gst-discoverer-1.0

### gst-launch-1.0
该工具接受管道的文本描述，将其实例化，并将其设置为播放状态。它允许您在使用GStreamer API调用实际实现之前快速检查给定管道是否有效。

请记住，它只能创建简单的管道。特别是，它只能模拟管道与应用程序在一定程度上的交互。在任何情况下，快速测试管道都非常方便，全世界的GStreamer开发人员每天都在使用它。

请注意，gst-launch-1.0主要是为开发人员提供的调试工具。你不应该在它之上构建应用程序。相反，可以使用GStreamer API的gst_parse_launch()函数，这是一种从管道描述构建管道的简单方法。

尽管构建管道描述的规则非常简单，但多个元素的连接可以迅速使此类描述变得像黑魔法一样。不要害怕，因为每个人最终都会学习gst-launch-1.0语法。

gst-launch-1.0的命令行通过`PIPELINE-DESCRIPTION`由一个选项列表组成。下面给出了一些简化的说明，gst-launch-1.0的参考页面提供了完整的文档。

### Elements
简单来说，PIPELINE-DESCRIPTION就是用感叹号(!)分隔的元素类型列表。输入下面的命令:

```bash
gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
```
你应该会看到一个带有动画视频模式的窗口。在终端上使用CTRL+C停止程序。

这将实例化一个类型为videotestsrc的新元素(一个生成示例视频模式的元素)，一个videoconvert(一个进行原始视频格式转换的元素，确保其他元素可以相互理解)，以及一个autovideosink(一个渲染视频的窗口)。然后，GStreamer尝试将每个元素的输出链接到描述中右侧元素的输入。如果有多个输入或输出Pad可用，则使用Pad cap找到两个兼容的Pad。

### 属性Properties
属性可以以*property=value *的形式添加到元素中(可以指定多个属性，用空格分隔)。使用gst-inspect-1.0工具(后面会解释)查找元素的可用属性。
```bash
gst-launch-1.0 videotestsrc pattern=11 ! videoconvert ! autovideosink
```
你会看到一个由圆圈组成的静态视频图案。

### 给元素命名Named elements
可以使用name属性为元素命名，这样就可以创建涉及分支的复杂管道。名称允许链接到前面在描述中创建的元素，并且对于使用具有多个输出pad的元素是必不可少的，例如demuxer或tee。

命名元素使用其名称后跟一个句点来引用。

```bash
gst-launch-1.0 videotestsrc ! videoconvert ! tee name=t ! queue ! autovideosink t. ! queue ! autovideosink
```

您应该看到两个视频窗口，显示相同的示例视频模式。如果你只看到一个窗口，请尝试移动它，因为它可能在第二个窗口的顶部。

这个例子实例化了一个videotestsrc，链接到一个videoconvert，链接到一个tee(请记住《Basic tutorial 7: 多线程和Pad可用性》中的内容，tee将通过输入Pad发送的所有内容复制到每个输出Pad)。tee被简单命名为` t `(使用name属性)，然后链接到queue和autovideosink。同一个tee指的是使用“t”。`(注意点)，然后链接到第二个queue。

要了解为什么必须使用queue，请看Basic tutorial 7: 多线程和Pad可用性。

### Pads
在链接link两个元素时，您可能希望直接指定Pad，而不是让GStreamer选择使用哪个Pad。为此，可以在元素名称(它必须是一个命名元素)后面加上一个点号和Pad名称。使用gst-inspect-1.0工具获取元素的pad名称。

例如，当你想从demuxer中取得一个特定的流时，这很有用:
>gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.video_00 ! matroskamux ! filesink location=sintel_video.mkv

使用souphttpsrc从互联网获取媒体文件，它是webm格式的(一种特殊的Matroska容器，请参阅基本教程2:GStreamer概念)。然后我们使用matroskademux打开容器。该媒体包含音频和视频，因此matroskademux将创建两个输出pad，名为video_00和audio_00。我们将video_00链接到一个matroskamux元素，以便将视频流重新打包到一个新的容器中，最后将其链接到一个filesink，它将把视频流写入名为“sintel_video”的文件。(location属性指定了文件的名称)。

总而言之，我们获取了一个webm文件，剥离了音频，并生成了一个包含视频的matroska文件。如果我们只想保留音频:

>gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.audio_00 ! vorbisparse ! matroskamux ! filesink location=sintel_audio.mka

vorbisparse元素用于从流中提取一些信息并将其放入Pad Caps中，因此下一个元素matroskamux知道如何处理流。对于视频来说，这是不必要的，因为matroskademux已经提取了该信息并将其添加到Caps中。

注意，在上面的两个例子中，没有任何媒体被解码或播放。我们刚刚从一个容器移动到另一个容器(再次解复用和再复用)。

### Caps filters
当一个元素有多个输出pad时，可能会出现到下一个元素的链接不明确的情况:下一个元素可能有多个兼容的输入pad，或者它的输入pad可能与所有输出pad的pad兼容。在这些情况下，GStreamer将使用可用的第一个pad进行链接，这几乎相当于说GStreamer将随机选择一个输出pad。

请看下面的管道:

>gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux ! filesink location=test

这和前面例子中的媒体文件和demuxer是一样的。filesink的输入Pad Caps是ANY，这意味着它可以接受任何类型的媒体。matroskademux的两个输出pad中的哪一个将链接到filesink?Video_00还是audio_00?你不可能知道。

不过，我们可以像前一节介绍的那样，使用命名补全(named pad)或Caps Filters来消除这种歧义:
>gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux ! video/x-vp8 ! matroskamux ! filesink location=sintel_video.mkv

`如果存在括号（），会提示语法错误，可以通过添加双引号解决`
eg.

>gst-launch-1.0 -v rtspsrc \
location=rtsp://admin:yangquan123@192.168.10.11:554/Streaming/Channels/101 ! \
rtph264depay ! h264parse ! nvv4l2decoder ! \
nvvideoconvert ! "video/x-raw(memory:NVMM),width=1280,height=720,format=I420" ! queue ! tee name=t ! \
queue ! nvv4l2h264enc ! h264parse ! qtmux ! filesink location=~/Desktop/file.mp4 t. ! \
queue ! nvegltransform ! nveglglessink window-width=1280 window-height=720 sync=false

Caps Filter的行为类似于传递元素，它什么都不做，只接受具有给定Caps的媒体，有效地解决了歧义。在这个例子中，在matroskademux和matroskamux之间，我们添加了一个video/x-vp8 Caps Filter，以指定我们对matroskademux的输出cap感兴趣，它可以产生这种视频。

使用gst-inspect-1.0工具可以找到元素接受和产生的Caps。要找出特定文件中包含的大写字母，请使用gst- discovery -1.0工具。要查看元素为特定管道生成的Caps值，可以像往常一样运行gst-launch-1.0，使用-v选项打印Caps信息。

### Examples
使用playbin播放媒体文件(就像在基础教程1:Hello world!中一样):
>gst-launch-1.0 playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm

一个完整的操作playback管道，包含音频和视频(与playbin内部创建的管道大致相同):

>gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d ! queue ! vp8dec ! videoconvert ! autovideosink d. ! queue ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink

一个转码管道，打开webm容器并解码两个流(通过uridecodebin)，然后用不同的编解码器重新编码音频和视频分支，并将它们重新放在Ogg容器中。

>gst-launch-1.0 uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm name=d ! queue ! theoraenc ! oggmux name=m ! filesink location=sintel.ogg d. ! queue ! audioconvert ! audioresample ! flacenc ! m.

缩放管道。当输入帧大小和输出帧大小不同时，videoscale元素会执行缩放操作。输出上限由caps Filter设置为320x200。

>gst-launch-1.0 uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! queue ! videoscale ! video/x-raw-yuv,width=320,height=200 ! videoconvert ! autovideosink

对gst-launch-1.0的简短描述应该足以让您入门。

## gst-inspect-1.0

该工具有三种操作模式:

 - 如果没有参数，它会列出所有可用的元素类型，也就是可用于实例化新元素的类型。
 - 使用文件名作为参数，它将该文件视为GStreamer插件，尝试打开它，并列出其中描述的所有元素。
 - 使用GStreamer元素名作为参数，它列出了与该元素有关的所有信息。

让我们看一个第三种模式的例子:

```bash
gst-inspect-1.0 vp8dec

Factory Details:
  Rank                     primary (256)
  Long-name                On2 VP8 Decoder
  Klass                    Codec/Decoder/Video
  Description              Decode VP8 video streams
  Author                   David Schleef <ds@entropywave.com>, Sebastian Dröge <sebastian.droege@collabora.co.uk>

Plugin Details:
  Name                     vpx
  Description              VP8 plugin
  Filename                 /usr/lib64/gstreamer-1.0/libgstvpx.so
  Version                  1.6.4
  License                  LGPL
  Source module            gst-plugins-good
  Source release date      2016-04-14
  Binary package           Fedora GStreamer-plugins-good package
  Origin URL               http://download.fedoraproject.org

GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstVideoDecoder
                         +----GstVP8Dec

Pad Templates:
  SINK template: 'sink'
    Availability: Always
    Capabilities:
      video/x-vp8

  SRC template: 'src'
    Availability: Always
    Capabilities:
      video/x-raw
                 format: I420
                  width: [ 1, 2147483647 ]
                 height: [ 1, 2147483647 ]
              framerate: [ 0/1, 2147483647/1 ]


Element Flags:
  no flags set

Element Implementation:
  Has change_state() function: gst_video_decoder_change_state

Element has no clocking capabilities.
Element has no URI handling capabilities.

Pads:
  SINK: 'sink'
    Pad Template: 'sink'
  SRC: 'src'
    Pad Template: 'src'

Element Properties:
  name                : The name of the object
                        flags: readable, writable
                        String. Default: "vp8dec0"
  parent              : The parent of the object
                        flags: readable, writable
                        Object of type "GstObject"
  post-processing     : Enable post processing
                        flags: readable, writable
                        Boolean. Default: false
  post-processing-flags: Flags to control post processing
                        flags: readable, writable
                        Flags "GstVP8DecPostProcessingFlags" Default: 0x00000403, "mfqe+demacroblock+deblock"
                           (0x00000001): deblock          - Deblock
                           (0x00000002): demacroblock     - Demacroblock
                           (0x00000004): addnoise         - Add noise
                           (0x00000400): mfqe             - Multi-frame quality enhancement
  deblocking-level    : Deblocking level
                        flags: readable, writable
                        Unsigned Integer. Range: 0 - 16 Default: 4
  noise-level         : Noise level
                        flags: readable, writable
                        Unsigned Integer. Range: 0 - 16 Default: 0
  threads             : Maximum number of decoding threads
                        flags: readable, writable
                        Unsigned Integer. Range: 1 - 16 Default: 0
```
最相关的部分是:

 - Pad Templates:列出了该元素可以拥有的所有Pad类型及其功能。在这里，你可以确定一个元素是否可以与另一个元素链接。在这种情况下，它只有一个sink pad模板，只接受视频/x-vp8 (VP8格式编码的视频数据)和一个src pad模板，产生video/x-raw(解码的视频数据)。
 - 元素属性(Element Properties):列出元素的属性，以及它们的类型和可接受值。

要了解更多信息，可以查看gst-inspect-1.0的文档页面。

## gst-discoverer-1.0
该工具是基本教程9:媒体信息收集中所示的GstDiscoverer对象的包装器。它从命令行中接受一个URI，并打印出GStreamer可以提取的所有有关媒体的信息。这有助于找出生成媒体时使用了什么容器和编解码器，从而确定需要在管道中放入什么元素才能播放它。

使用gst-discoverer-1.0 --help获取可用选项列表，这基本上控制了输出的详细程度。

让我们看一个例子:
```bash
gst-discoverer-1.0 https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm -v

Analyzing https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
Done discovering https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
Topology:
  container: video/webm
    audio: audio/x-vorbis, channels=(int)2, rate=(int)48000
      Codec:
        audio/x-vorbis, channels=(int)2, rate=(int)48000
      Additional info:
        None
      Language: en
      Channels: 2
      Sample rate: 48000
      Depth: 0
      Bitrate: 80000
      Max bitrate: 0
      Tags:
        taglist, language-code=(string)en, container-format=(string)Matroska, audio-codec=(string)Vorbis, application-name=(string)ffmpeg2theora-0.24, encoder=(string)"Xiph.Org\ libVorbis\ I\ 20090709", encoder-version=(uint)0, nominal-bitrate=(uint)80000, bitrate=(uint)80000;
    video: video/x-vp8, width=(int)854, height=(int)480, framerate=(fraction)25/1
      Codec:
        video/x-vp8, width=(int)854, height=(int)480, framerate=(fraction)25/1
      Additional info:
        None
      Width: 854
      Height: 480
      Depth: 0
      Frame rate: 25/1
      Pixel aspect ratio: 1/1
      Interlaced: false
      Bitrate: 0
      Max bitrate: 0
      Tags:
        taglist, video-codec=(string)"VP8\ video", container-format=(string)Matroska;

Properties:
  Duration: 0:00:52.250000000
  Seekable: yes
  Tags:
      video codec: VP8 video
      language code: en
      container format: Matroska
      application name: ffmpeg2theora-0.24
      encoder: Xiph.Org libVorbis I 20090709
      encoder version: 0
      audio codec: Vorbis
      nominal bitrate: 80000
      bitrate: 80000

```
## 结论
本教程展示了:

 - 如何使用gst-launch-1.0工具从命令行构建和运行GStreamer管道。
 - 如何使用gst-inspect-1.0工具找出可用的GStreamer元素及其功能。
 - 如何使用gst- discovery -1.0发现媒体文件的内部结构。

很高兴在这里见到你，希望很快见到你!