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

int
main(int argc, char *argv[])
{
  GstElement *playbin;
  GstElement *x_overlay;

  GdkWindow *video_window_xwindow;
  GtkWidget *window, *video_window;
  gulong embed_xid;

  gtk_init(&argc, &argv);
  gst_init(&argc, &argv);

  playbin = gst_element_factory_make ("playbin", "playbin");
  x_overlay = gst_element_factory_make("xvimagesink", "videosink");

  if (!playbin) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Set the URI to play */
  g_object_set (playbin, "uri", "file:///home/lieryang/Desktop/gst-study/sample-480p.webm", NULL);
  g_object_set (playbin, "video-sink", x_overlay, NULL);
  //g_object_set (G_OBJECT(x_overlay), "window-width", 1920, "window-height", 1080, NULL);
  
  /* prepare the ui */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);   

  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
  gtk_window_set_title (GTK_WINDOW (window), "GstVideoOverlay Gtk+ demo");

  video_window = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (window), video_window);
  gtk_container_set_border_width (GTK_CONTAINER (window), 2);

  gtk_widget_show_all (window);

  video_window_xwindow = gtk_widget_get_window (video_window);
  embed_xid = GDK_WINDOW_XID (video_window_xwindow);
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (x_overlay), embed_xid);

  gst_element_set_state (playbin, GST_STATE_PLAYING);
  
  gtk_main();

  return 0;
}