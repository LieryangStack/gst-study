# 02 Specifying the pads

如前所述，填充区是数据进出元素的端口，这使得它们在元素创建过程中非常重要。在样板代码中，我们看到了静态pad模板如何使用element类注册pad模板。在这里，我们将看到如何创建实际的元素，使用_event()函数为特定的格式进行配置，以及如何注册函数以让数据流经元素。

在element _init()函数中，根据_class_init()函数中的element类注册的pad模板创建pad。在创建pad之后，必须设置_chain()函数指针，它将在sinkpad上接收和处理输入数据。你也可以选择设置一个_event()函数指针和一个_query()函数指针。此外，pad也可以在循环模式下工作，这意味着它们可以自己拉取数据。稍后会详细讨论这个话题。之后，您必须将pad注册到元素。事情是这样的:

```c
static void
gst_my_filter_init (GstMyFilter *filter)
{
  /* pad through which data comes in to the element */
  filter->sinkpad = gst_pad_new_from_static_template (
    &sink_template, "sink");
  /* pads are configured here with gst_pad_set_*_function () */



  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  /* pad through which data goes out of the element */
  filter->srcpad = gst_pad_new_from_static_template (
    &src_template, "src");
  /* pads are configured here with gst_pad_set_*_function () */



  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  /* properties initial value */
  filter->silent = FALSE;
}

```

## 参考
[翻译自：Specifying the pads](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/pads.html?gi-language=c)