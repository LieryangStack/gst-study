#include <X11/Xlib.h>
#include <gst/gst.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>
#include <gst/video/videooverlay.h>


GstElement *pipeline, *source, *conv, *queue, *sink;
Display *display;
Window win;

void
realize (GtkWidget* self,
         gpointer user_data ) {
  GdkSurface *surface = gtk_native_get_surface (gtk_widget_get_native (GTK_WIDGET (self)));
  XID x_window = gdk_x11_surface_get_xid(surface);

  
  g_print ("x_window =%ld\n", x_window);
  XMapWindow(display, x_window);
}

int 
main() {
  gst_init(NULL, NULL);
  gtk_init();

  source = gst_element_factory_make ("videotestsrc", NULL);
  queue = gst_element_factory_make ("queue", NULL);
  conv = gst_element_factory_make ("videoconvert", NULL);
  sink = gst_element_factory_make ("nveglglessink", NULL);

  pipeline = gst_pipeline_new ("test-pipeline");

  /* 判断pipeline和Element是否创建成功 */
  if (!pipeline || !source || !conv || !queue || !sink ) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many (GST_BIN (pipeline), source, conv,  \
                                        queue, sink, NULL);

  if (gst_element_link_many (source, queue, conv, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /**
   * XOpenDisplay()是X Window系统中用于创建一个客户端和X服务器之间的连接的函数
   * 它的参数是一个字符串，通常是'NULL'或者是指向服务器的名称
  */
  display = XOpenDisplay(NULL);
  if (display == NULL){
    g_print("Cannot open display\n");
    return 1;
  }

  guint width = 480;
  guint height = 480;
  /**
   * @param 1: 连接到 X 服务器的 Display 指针
   * @param 2: 新创建窗口的父窗口 ID。如果该值为 None，则新窗口将成为根窗口
   * @param 3 @param 4: 新窗口的左上角在父窗口或根窗口坐标系中的位置坐标
   * @param 5 @param 6: 新窗口的宽度和高度
   * @param 7: 新窗口边框的宽度
   * @param 8: 新窗口边框的颜色。它是一个 unsigned long 类型的值，表示颜色的 RGB 值
   * @param 9: 新窗口背景的颜色。它也是一个 unsigned long 类型的值，表示颜色的 RGB 值
  */
  win = XCreateSimpleWindow(display, DefaultRootWindow(display), 
                               300, 300, width, height, 0, 0, 0);
  
  /* 设置窗口属性 */
  XSetWindowAttributes attr;
  /*  设置为 True，表示忽略窗口管理器的控制，意味着窗口管理器不会为该窗口提供装饰（例如标题栏、边框等） */
  attr.override_redirect = True;
  /* 本质就是把CWOverrideRedirect设定为了True */
  XChangeWindowAttributes(display, win, CWOverrideRedirect, &attr);

  /* 显示窗口 */
  XMapWindow(display, win);

  /* 设置窗口总是在顶层 */
  XEvent e;
  e.xclient.type = ClientMessage;
  e.xclient.serial = 0;
  e.xclient.send_event = True;
  e.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
  e.xclient.format = 32;
  e.xclient.data.l[0] = 1;
  e.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", False);
  e.xclient.data.l[2] = 0;

  XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, &e);

  /* 必须在播放前进行设定 */
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink),
                                         (gulong) win);

  guint ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
  }

  XWindowAttributes attrs;
  int x, y;
  /* 隐藏窗口 */
  XUnmapWindow(display, win);

  // for (gint i = 0; i < 100000; i++) {
  //   g_print ("i = %d\n", i);
  //   if (i % 1000 == 0) {
  //     XGetWindowAttributes(display, win, &attrs);
  //     x = attrs.x;
  //     y = attrs.y;
  //     width = attrs.width;
  //     height = attrs.height;
  //     XMoveWindow(display, win, x + 10, y);
  //     XResizeWindow(display, win, width + 10, height);
  //   }
  // }
  /* 显示窗口 */
  XMapWindow(display, win);

  GtkWidget *picture = NULL;
  GtkWidget *window = NULL;
  GtkWidget *box = NULL;

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
  // 进入事件循环
  // XEvent event;
  // while (1) {
  //   XNextEvent(display, &event);
  // }

  // 关闭 X11 显示连接
  XCloseDisplay(display);
  return 0;
}
