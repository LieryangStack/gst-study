#include <string.h>

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

int main (int argc, char **argv)
{
  GdkWindow *video_window_xwindow;
  GtkWidget *window, *video_window;
  gulong embed_xid;
  GstStateChangeReturn sret;

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  GstElement *pipeline, *udpsrc, *appxrtp, *depay, *parse, *omxh264dec, *videoConvert, *sink;

  pipeline = gst_pipeline_new ("xvoverlay");
  udpsrc = gst_element_factory_make ("udpsrc", NULL); g_assert(udpsrc);

  //Set CAPS
  g_object_set (G_OBJECT (udpsrc), "port", 6000, NULL);
  GstCaps * xrtpcaps = gst_caps_from_string("application/x-rtp,encoding-name=H264");
  g_object_set (udpsrc, "caps", xrtpcaps, NULL);

  depay = gst_element_factory_make ("rtph264depay", NULL); g_assert(depay);
  parse = gst_element_factory_make ("h264parse", NULL); g_assert(parse);
  omxh264dec = gst_element_factory_make ("omxh264dec", NULL); g_assert(omxh264dec);
  videoConvert = gst_element_factory_make ("videoconvert", NULL); g_assert(videoConvert);
  sink = gst_element_factory_make ("xvimagesink", NULL); g_assert(sink);

  //ADD
  gst_bin_add_many (GST_BIN (pipeline), udpsrc, depay, parse, omxh264dec, videoConvert, sink, NULL);

  //LINK
  g_assert(gst_element_link (udpsrc, depay));
  g_assert(gst_element_link (depay, parse));
  g_assert(gst_element_link (parse, omxh264dec));
  g_assert(gst_element_link (omxh264dec, videoConvert));
  g_assert(gst_element_link (videoConvert, sink));

  /* prepare the ui */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);   

  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_main_quit), (gpointer) pipeline);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
  gtk_window_set_title (GTK_WINDOW (window), "GstVideoOverlay Gtk+ demo");

  video_window = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (window), video_window);
  gtk_container_set_border_width (GTK_CONTAINER (window), 2);

  gtk_widget_show_all (window);

  video_window_xwindow = gtk_widget_get_window (video_window);
  embed_xid = GDK_WINDOW_XID (video_window_xwindow);
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink), embed_xid);

  /* run the pipeline */
  sret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (sret == GST_STATE_CHANGE_FAILURE)
    gst_element_set_state (pipeline, GST_STATE_NULL);
  else
    gtk_main ();

  gst_object_unref (pipeline);
  return 0;
}