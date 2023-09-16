#include<string.h>
#include<gst/gst.h>
#include<gst/pbutils/pbutils.h>

typedef struct _CustomData{
  GstDiscoverer *discoverer;
  GMainLoop *loop;
}CustomData;

static void
print_tag_foreach(const GstTagList *tags, const gchar *tag, gpointer user_data){
  GValue val = G_VALUE_INIT;

  gchar *str;
  gint depth = GPOINTER_TO_INT(user_data);

  gst_tag_list_copy_value(&val, tags, tag);
  if(G_VALUE_HOLDS_STRING(&val))
    str = g_value_dup_string(&val);
  else
    str = gst_value_serialize(&val);

  g_print("%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick(tag), str);
  g_free(str);

  g_value_unset(&val);
}

static void
print_stream_info(GstDiscovererInfo *info, gint depth){
  gchar *desc = NULL;
  GstCaps *caps;
  const GstTagList *tags;

  caps = gst_discoverer_stream_info_get_caps(info);

  if(caps){
    if(gst_caps_is_fixed(caps))
      desc = gst_pb_utils_get_codec_description(caps);
    else
      desc = gst_caps_to_string(caps);
    gst_caps_unref(caps);
  }
  

}

static void 
on_discovered_cb(GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, CustomData *data){

}

int main(int argc, char *argv[]){
  CustomData data;
  GError *err = NULL;
  gchar *uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

  if(argc > 1){
    uri = argv[1];
  }

  memset(&data, 0, sizeof(data));

  gst_init(&argc, &argv);

  g_print("Discovering '%s'\n", uri);

  data.discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
  if(!data.discoverer){
    g_print("Error creating discoverer instance: %s\n", err->message);
    g_clear_error(&err);
    return -1;
  }

  g_signal_connect(data.discoverer, "discovered", G_CALLBACK(on_))

  return FALSE;
}

