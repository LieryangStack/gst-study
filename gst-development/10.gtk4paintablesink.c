#include <gst/gst.h>
#include <gtk/gtk.h>

GMainContext *main_context = NULL;
GdkPaintable *paintable = NULL;
GtkWidget *picture = NULL;
GstElement *pipeline, *source, *conv, *sink;
static int flag = 1;

static gboolean
set_paintable (gpointer user_data)
{
  g_object_get (sink, "paintable", &paintable, NULL);
  g_message ("paintable = %p\n", paintable);
  gtk_picture_set_paintable (GTK_PICTURE (picture), paintable);

  return G_SOURCE_REMOVE;
}

void *
gst_thread (gpointer data) {
  GMainContext *gst_main_context = g_main_context_new ();
  g_main_context_push_thread_default (gst_main_context);
  GMainLoop *gst_main_loop = g_main_loop_new (gst_main_context, FALSE);

  gst_init(NULL, NULL);
  source = gst_element_factory_make ("videotestsrc", NULL);
  conv = gst_element_factory_make ("videoconvert", NULL);
  sink = gst_element_factory_make ("gtk4paintablesink", NULL);
  
  pipeline = gst_pipeline_new ("test-pipeline");
  
  if (!pipeline || !source || !sink || !conv ) {
    g_printerr ("1Not all elements could be created.\n");
    return NULL;
  }

  gst_bin_add_many (GST_BIN (pipeline), source, conv, sink, NULL);

  if (gst_element_link_many (source, conv, sink, NULL) != TRUE) {
    g_printerr ("2Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return NULL;
  }
  guint ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return NULL;
  }
  flag = 0;

  g_idle_add (set_paintable, data);

  g_main_loop_run (gst_main_loop);
  g_main_loop_unref (gst_main_loop);
}

static void
button_clicked (GtkButton *button,
                gpointer  user_data) {
  g_message ("%s\n", __func__);

  g_thread_new ("gst.test", gst_thread, NULL);
}

static void 
app_activate (GApplication *app, gpointer *user_data) {
  GtkWidget *win = NULL;
  GtkWidget *box = NULL;
  GtkWidget *button = NULL;

  win = gtk_application_window_new (GTK_APPLICATION (app));
  gtk_window_set_title (GTK_WINDOW (win), "gtk4-test");
  gtk_window_set_default_size (GTK_WINDOW (win), 500, 400);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW (win), box);

  picture = gtk_picture_new ();
  gtk_box_append (GTK_BOX (box), picture);

  button = gtk_button_new ();
  gtk_box_append (GTK_BOX (box), button);

  g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), picture);

  gtk_window_present (GTK_WINDOW (win));
}


int
main (int argc, char *argv[]) {
  GtkApplication *app;
  int state;
  gst_init(NULL, NULL);

  app = gtk_application_new ("gtk4.test",G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
  state = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return 0;
}
