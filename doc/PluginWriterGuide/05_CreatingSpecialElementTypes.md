# 5 Creating special element types

到目前为止，我们已经深入探讨了可以嵌入到GStreamer元素中的几乎所有功能。大部分内容都是相当低级的，深入剖析了GStreamer的内部工作原理。幸运的是，GStreamer包含了一些更容易使用的接口来创建这些元素。为了做到这一点，我们将更仔细地查看GStreamer提供的基类的元素类型（源、接收器和转换元素）。我们还将更仔细地查看一些不需要特定编码的元素类型，比如调度交互或数据传递，而是需要特定的管道控制的元素类型（例如N-to-1元素和管理器）。

[Pre-made base classes](./05_CreatingSpecialElementTypes/01_Pre-made_base_classes.md)

[Writing a Demuxer or Parser](./05_CreatingSpecialElementTypes/02_Writing_a_Demuxer_or_Parser.md)

[Writing a N-to-1 Element or Muxer](./05_CreatingSpecialElementTypes/03_Writing_a-N-to-1_Element_or_Muxer.md)

[Writing a Manager](./05_CreatingSpecialElementTypes/04_Writing_a_Manager.md)