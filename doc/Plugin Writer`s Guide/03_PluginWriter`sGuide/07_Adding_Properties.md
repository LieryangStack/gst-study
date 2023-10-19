# 7 Adding Properties
控制元素行为的最主要也是最重要的方式，就是通过GObject属性。GObject属性在_class_init()函数中定义。元素可选地实现_get_property()和_set_property()函数。如果应用程序更改或请求属性的值，这些函数将收到通知，然后可以填充该值或采取该属性内部更改值所需的操作。

你可能还想在get和set函数中使用的属性的当前配置值周围保存一个实例变量。注意，GObject不会自动将实例变量设置为默认值，您必须在元素的_init()函数中这样做。

```c
/* properties */
enum {
  PROP_0,
  PROP_SILENT
  /* FILL ME */
};

static void gst_my_filter_set_property  (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec);
static void gst_my_filter_get_property  (GObject      *object,
                         guint         prop_id,
                         GValue       *value,
                         GParamSpec   *pspec);

static void
gst_my_filter_class_init (GstMyFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* define virtual function pointers */
  object_class->set_property = gst_my_filter_set_property;
  object_class->get_property = gst_my_filter_get_property;

  /* define properties */
  g_object_class_install_property (object_class, PROP_SILENT,
    g_param_spec_boolean ("silent", "Silent",
              "Whether to be very verbose or not",
              FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_my_filter_set_property (GObject      *object,
                guint         prop_id,
                const GValue *value,
                GParamSpec   *pspec)
{
  GstMyFilter *filter = GST_MY_FILTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      g_print ("Silent argument was changed to %s\n",
           filter->silent ? "true" : "false");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_my_filter_get_property (GObject    *object,
                guint       prop_id,
                GValue     *value,
                GParamSpec *pspec)
{
  GstMyFilter *filter = GST_MY_FILTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

```
以上是如何使用属性的一个非常简单的例子。图形化应用程序将使用这些属性，并显示一个用户可控制的小部件，通过它可以更改这些属性。这意味着——为了让属性尽可能对用户友好——你应该尽可能精确地定义属性。不仅可以定义可以放置有效属性的范围(对于整数、浮点数等)，还可以在属性定义中使用非常具有描述性(最好是国际化)的字符串，如果可能的话，可以使用枚举和flags而不是整数。GObject文档以非常完整的方式描述了这些，但下面，我们将给出一个简短的示例，说明这在哪里有用。请注意，在这里使用整数可能会让用户完全迷惑，因为它们在这个上下文中没有任何意义。这个例子是从videotestsrc中拷贝来的。

```c
typedef enum {
  GST_VIDEOTESTSRC_SMPTE,
  GST_VIDEOTESTSRC_SNOW,
  GST_VIDEOTESTSRC_BLACK
} GstVideotestsrcPattern;

[..]

#define GST_TYPE_VIDEOTESTSRC_PATTERN (gst_videotestsrc_pattern_get_type ())
static GType
gst_videotestsrc_pattern_get_type (void)
{
  static GType videotestsrc_pattern_type = 0;

  if (!videotestsrc_pattern_type) {
    static GEnumValue pattern_types[] = {
      { GST_VIDEOTESTSRC_SMPTE, "SMPTE 100% color bars",    "smpte" },
      { GST_VIDEOTESTSRC_SNOW,  "Random (television snow)", "snow"  },
      { GST_VIDEOTESTSRC_BLACK, "0% Black",                 "black" },
      { 0, NULL, NULL },
    };

    videotestsrc_pattern_type =
    g_enum_register_static ("GstVideotestsrcPattern",
                pattern_types);
  }

  return videotestsrc_pattern_type;
}

[..]

static void
gst_videotestsrc_class_init (GstvideotestsrcClass *klass)
{
[..]
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PATTERN,
    g_param_spec_enum ("pattern", "Pattern",
               "Type of test pattern to generate",
                       GST_TYPE_VIDEOTESTSRC_PATTERN, GST_VIDEOTESTSRC_SMPTE,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
[..]
}


```