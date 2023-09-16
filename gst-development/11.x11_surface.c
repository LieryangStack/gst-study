#include <gst/gst.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>
#include <gst/video/videooverlay.h>

GstElement *pipeline, *source, *conv, *tee, *queue1, *queue2, *gtksink, *autosink;
GstElement *conv1, *conv2;
int flag = 1;

GtkWidget *picture = NULL;
GtkWidget *window = NULL;
GtkWidget *box = NULL;


void *
gst_thread (gpointer data) {
  gst_init(NULL, NULL);
  guint ret;

  source = gst_element_factory_make ("videotestsrc", NULL);
  conv = gst_element_factory_make ("videoconvert", NULL);
  tee = gst_element_factory_make ("tee", NULL);

  queue2 = gst_element_factory_make ("queue", NULL);
  conv2 = gst_element_factory_make ("videoconvert", NULL);
  autosink = gst_element_factory_make ("nveglglessink", NULL);

  pipeline = gst_pipeline_new ("test-pipeline");

  /* 判断pipeline和Element是否创建成功 */
  if (!pipeline || !source || !conv || !tee || \
      !queue2 || !autosink || !conv2 ) {
    g_printerr ("1Not all elements could be created.\n");
    return NULL;
  }

  gst_bin_add_many (GST_BIN (pipeline), source, conv, tee, \
                    queue2, autosink, conv2, NULL);

  if (gst_element_link_many (source, conv, tee, NULL) != TRUE) {
    g_printerr ("2Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return NULL;
  }

  if (gst_element_link_many(queue2, conv2, autosink, NULL) != TRUE) {
    g_printerr ("3BElements could not be linked.\n");
    gst_object_unref (pipeline);
    return NULL;
  }

  GstPad *src_pad = gst_element_request_pad_simple (tee, "src_%u");
  GstPad *sink_pad = gst_element_get_static_pad (queue2, "sink");
  gst_pad_link (src_pad, sink_pad);
  gst_object_unref (sink_pad);
  gst_object_unref (src_pad);

  ret = gst_element_set_state (pipeline, GST_STATE_READY);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
  }
}

void
realize (GtkWidget* self,
         gpointer user_data ) {
  GdkSurface *surface = gtk_native_get_surface (gtk_widget_get_native (GTK_WIDGET (self)));
  XID x_window = gdk_x11_surface_get_xid(surface);

  
  g_print ("x_window =%ld\n", x_window);
  GstVideoOverlay *overlay = GST_VIDEO_OVERLAY(autosink);
  gst_video_overlay_set_window_handle (overlay, x_window);

  guint ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
  }
}


int
main (int argc, char *argv[]) {

  gtk_init();
  gst_thread (NULL);

  window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(window), "GTK4 PaintableSink Demo");
  gtk_widget_set_size_request(window, 680, 680);
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  //gtk_window_set_child (GTK_WINDOW (window), box);
  
  GtkWidget *button = gtk_button_new_with_label ("button");
  GtkWidget *drawing = gtk_drawing_area_new ();

  gtk_widget_set_hexpand (drawing, TRUE);
  gtk_widget_set_vexpand (drawing, TRUE);

  gtk_box_append (GTK_BOX (box), drawing);
  gtk_box_append (GTK_BOX (box), button);

  g_signal_connect(window, "realize", G_CALLBACK(realize), NULL);

  gtk_window_present (GTK_WINDOW (window));

  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
