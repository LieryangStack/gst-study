#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<stdlib.h>
#include<glib.h>
#include<glib/gprintf.h>

void
channel_unix_new_test(void){
  g_print("%s()in\n",__func__);
  int fd = open("12.test.txt", O_RDONLY);
  GIOChannel *giofile = g_io_channel_unix_new(fd);

  gchar *str_return;
  gsize length, terminator_pos;
  GError *error = NULL;
  GIOStatus giostatus = g_io_channel_read_line(giofile,
                                               &str_return,
                                               &length,
                                               &terminator_pos,
                                               &error);
  if(giostatus == G_IO_STATUS_NORMAL)
    g_print("read_line:%s, length:%ld, terminator_pos:%ld\n", str_return, length, terminator_pos);
  else if(giostatus == G_IO_STATUS_ERROR)
    g_print("read_line error G_IO_STATUS_ERROR:%s\n", error->message);
  else if(giostatus == G_IO_STATUS_EOF)
    g_print("read_line error G_IO_STATUS_EOF:%s\n", error->message);
  else if(giostatus == G_IO_STATUS_AGAIN)
    g_print("read_line error G_IO_STATUS_AGAIN:%s\n", error->message);
 
    GString *buffer = g_string_new (NULL);
    giostatus = g_io_channel_read_line_string (giofile,
                                               buffer,
                                               &terminator_pos,
                                               &error);
    if (giostatus == G_IO_STATUS_NORMAL)
        g_printf ("read_line_string:%s\n", buffer->str);
    else if (giostatus == G_IO_STATUS_ERROR)
        g_printf ("read_line_string error G_IO_STATUS_ERROR:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_EOF)
        g_printf ("read_line_string error G_IO_STATUS_EOF:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_AGAIN)
        g_printf ("read_line_string error G_IO_STATUS_AGAIN:%s\n", error->message);
    g_string_free (buffer, TRUE);
    
    giostatus = g_io_channel_read_to_end (giofile,
                                          &str_return,
                                          &length,
                                          &error);
    if (giostatus == G_IO_STATUS_NORMAL)
        g_printf ("read_to_end:%s, length:%ld\n", str_return, length);
    else if (giostatus == G_IO_STATUS_ERROR)
        g_printf ("read_to_end error G_IO_STATUS_ERROR:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_EOF)
        g_printf ("read_to_end error G_IO_STATUS_EOF:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_AGAIN)
        g_printf ("read_to_end error G_IO_STATUS_AGAIN:%s\n", error->message);
    
    gint gfd = g_io_channel_unix_get_fd (giofile);
    g_io_channel_shutdown (giofile, TRUE, &error);
    close(gfd);
    g_print ("%s() out\n", __func__);
}

void 
channel_read_chars_test(void){
  g_print("%s() in\n", __func__);
  GError *error = NULL;
  GIOChannel *giofile = g_io_channel_new_file("12.test.txt",
                                              "r",
                                              &error);
  gchar *buf = g_new0(gchar, 1024);
  gsize bytes_read;
  GIOStatus giostatus = g_io_channel_read_chars(giofile,
                                                buf,
                                                1024,
                                                &bytes_read,
                                                &error);
  if(giostatus == G_IO_STATUS_NORMAL)
    g_print("read_chars:%s, length:%ld\n", buf, bytes_read);
  else if(giostatus == G_IO_STATUS_ERROR)
    g_print("read_chars error G_IO_STATUS_ERROR:%s\n", error->message);
  else if(giostatus == G_IO_STATUS_EOF)
    g_print("read_chars error G_IO_STATUS_EOF:%s\n", error->message);
  else if(giostatus == G_IO_STATUS_AGAIN)
    g_print("read_chars error G_IO_STATUS_AGAIN:%s\n", error->message);
  
  g_io_channel_shutdown(giofile, TRUE, &error);
  g_free(buf);
  g_print("%s() out\n", __func__);
}

void
channel_read_unichar_test(void){
  g_print("%s() in\n", __func__);
  GError *error = NULL;
  GIOChannel *giofile = g_io_channel_new_file("12.ansi.txt",
                                              "r",
                                              &error);
  GError *err = NULL;
  GIOStatus giostatus = g_io_channel_set_encoding(giofile,
                                                  "ANSI",
                                                   &error);
  gunichar thechar;
  giostatus = g_io_channel_read_unichar(giofile,
                                        &thechar,
                                        &error);
  if(giostatus == G_IO_STATUS_NORMAL)
    g_print("read_unichar:%d\n",thechar);
  else
    g_print("error");
  g_io_channel_shutdown(giofile, TRUE, &error);
  g_print("%s() out\n", __func__);
}

void channel_write_chars_test(void)
{
    g_printf ("%s() in\n", __func__);
    GError *error = NULL;
    GIOChannel *giofile = g_io_channel_new_file ("12.write.txt",
                                                 "a+",
                                                 &error);
    gsize bytes_written;
    GIOStatus giostatus = g_io_channel_write_chars (giofile,
                                                      "g_io_channel_write_chars",
                                                      -1,
                                                      &bytes_written,
                                                      &error);
    if (giostatus == G_IO_STATUS_ERROR)
        g_printf ("write_chars error G_IO_STATUS_ERROR:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_EOF)
        g_printf ("write_chars error G_IO_STATUS_EOF:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_AGAIN)
        g_printf ("write_chars error G_IO_STATUS_AGAIN:%s\n", error->message);
    giostatus = g_io_channel_flush (giofile,
                                    &error);
    if (giostatus == G_IO_STATUS_ERROR)
        g_printf ("flush error G_IO_STATUS_ERROR:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_EOF)
        g_printf ("flush error G_IO_STATUS_EOF:%s\n", error->message);
    else if (giostatus == G_IO_STATUS_AGAIN)
        g_printf ("flush error G_IO_STATUS_AGAIN:%s\n", error->message);
    g_io_channel_shutdown (giofile, TRUE, &error);
    g_printf ("%s() out\n", __func__);
}

void
FifoDestroyNotify(gpointer data){
  g_print("FifoDestroyNotify data: %s\n", (gchar *)data);
}

gboolean
TimeoutSourceFunc(gpointer user_data){
  g_print("%s() in\n", __func__);
  GMainLoop *loop = (GMainLoop *)user_data;
  g_main_loop_quit(loop);
  return FALSE;
}

gboolean
GIOFifoFunc(GIOChannel  *source, GIOCondition  condition, gpointer      data){
  g_print("GIOFifoFunc data:%s\n", (gchar *)data);
  g_print("condition:%d\n", condition);
  if(condition & G_IO_HUP){
    g_error("error:Pipe Disconnected!\n");
  }
  GError *error = NULL;
  gsize bytes_read;
  gsize length, terminator_pos;
  gchar *str_return;
  GIOStatus giostatus = g_io_channel_read_line(source,
                                               &str_return,
                                               &length,
                                               &terminator_pos,
                                               &error);
  if (giostatus == G_IO_STATUS_NORMAL)
    g_printf ("read_line:%s, length:%ld, terminator_pos:%ld\n", str_return, length, terminator_pos);
  else if (giostatus == G_IO_STATUS_ERROR)
    g_printf ("read_chars error G_IO_STATUS_ERROR:%s\n", error->message);
  else if (giostatus == G_IO_STATUS_EOF)
    g_printf ("read_chars error G_IO_STATUS_EOF:%s\n", error->message);
  else if (giostatus == G_IO_STATUS_AGAIN)
    g_printf ("read_chars error G_IO_STATUS_AGAIN:%s\n", error->message);
  g_free (str_return);
  return TRUE;
}           

void 
setup_child(gint input[]){
  g_print("%s() in\n", __func__);
  GIOChannel *channel_read;
  gchar *user_data = "test user data!";

  /*关闭不必要的pipe输入*/
  close(input[1]);
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  channel_read = g_io_channel_unix_new(input[0]);
  if(channel_read == NULL){
    g_error("error:Unable to establish GIOChannels!\n");
  }

  guint source_id = g_io_add_watch_full(channel_read,
                                        G_PRIORITY_DEFAULT,
                                        G_IO_IN|G_IO_HUP,
                                        GIOFifoFunc,
                                        (gpointer)user_data,
                                        FifoDestroyNotify);
  if(source_id == 0){
    g_error("error:Unable to add watch!\n");
  }

  g_timeout_add(1000, TimeoutSourceFunc, (gpointer)loop);
  g_main_loop_run(loop);
  g_source_remove(source_id);
  sleep(1);/*睡眠一秒*/
}

void setup_parent(gint output[]){
  g_print("%s() in\n", __func__);
  sleep(1);
  GIOChannel *channel_write;
  GError *error = NULL;
  gsize bytes_written;
  close(output[0]);

  channel_write = g_io_channel_unix_new(output[1]);
  if(channel_write == NULL){
    g_error("error not found GIOChannels!\n");
  }

  GIOStatus giostatus = g_io_channel_write_chars (channel_write,
                                                  "setup parent\n",
                                                  -1,
                                                  &bytes_written,
                                                  &error);
  giostatus = g_io_channel_flush (channel_write, &error);
  if (giostatus == G_IO_STATUS_ERROR)
      g_printf ("flush error G_IO_STATUS_ERROR:%s\n", error->message);
  else if (giostatus == G_IO_STATUS_EOF)
      g_printf ("flush error G_IO_STATUS_EOF:%s\n", error->message);
  else if (giostatus == G_IO_STATUS_AGAIN)
      g_printf ("flush error G_IO_STATUS_AGAIN:%s\n", error->message);
  sleep(5);
}

void
io_add_watch(void){
  g_print("%s() in\n", __func__);
  gint parent_to_child[2];

  /* 内核中开辟出一块缓冲区（称为管道），一个用于读端、一个用于写端
   * pipe函数调用成功返回 0
   * pipe函数调用失败返回 -1
   */
  if(pipe(parent_to_child) == -1){
    g_error("Error:%s\n", g_strerror(errno));
  }

  switch(fork()) {
    case -1:
      g_error("error:%s\n", g_strerror(errno));
      break;
    case 0: // 这是子程序
      setup_child(parent_to_child);
      break;
    default: // 这是父程序
      setup_parent(parent_to_child);
  }
  
  g_printf ("%s() out\n", __func__);
}


int
main(int argc, char *argv[]){
/*
  channel_unix_new_test();
  channel_read_chars_test();
  channel_read_unichar_test();
  channel_write_chars_test();
*/
  io_add_watch();

  return 0;
}