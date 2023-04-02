#include <glib.h>
#include <gio/gio.h> /* gio channel */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


gboolean 
client_recv (GIOChannel   *source, 
             GIOCondition  condition,
			       gpointer      data){
  gchar buff[128];
  gint recv_byte = 128;
  gint recv_sizing;

  gint clienct_insock = g_io_channel_unix_get_fd(source);
  recv_sizing = recv(clienct_insock, buff, recv_byte, 0);
  if(recv_sizing < 0 || recv_sizing == 0){
    g_print("connection lost or error while recv(); [just guess] number : %d\n", recv_sizing);
    close(clienct_insock);
    return FALSE;
  }

  buff[recv_sizing] = '\0';
  g_print("data: %s\n", buff);
  return TRUE;
}


gboolean
deal(GIOChannel *in, GIOCondition condition, gpointer data){
  
  struct sockaddr_in income;
  gint insock = g_io_channel_unix_get_fd(in);
  socklen_t income_len = sizeof(income);
  /*接受请求*/
  gint newsock = accept(insock, (struct sockaddr*)&income, &income_len);
  if(newsock == -1){
    g_print("failure on newsock\n");
  } 

  g_print("Linked successfully\n");

  /*得到客户端描述字*/
  GIOChannel *client_in = g_io_channel_unix_new(newsock);
  /*添加到主循环事件*/
  g_io_add_watch(client_in,
                 G_IO_IN | G_IO_OUT | G_IO_HUP,
                 (GIOFunc)client_recv,
                 NULL);
  return TRUE;
}

int
main(int argc, char *argv[]){
  GIOChannel *in;

  /*打开一个网络通讯端口，如果成功的话就像open()一样返回一个文件描述符*/
  gint rsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(rsock < 0){
    g_printerr("socket\n");
    return 1;
  }

  /*绑定端口号*/
  struct sockaddr_in my;
  my.sin_family = AF_INET;/* 套接口地址结构的地址 */
  my.sin_addr.s_addr = inet_addr("10.112.86.5");
  my.sin_port = htons(8080);
  gint ret = bind(rsock, (struct sockaddr *)&my, sizeof(my));
  if(ret < 0){
    g_printerr("bind\n");
    return 1;
  }

  /*开始监听*/
  ret = listen(rsock, 10);
  if(ret < 0){
    g_printerr("listen\n");
    return 1;
  }

  /*通过文件描述字得到GIOChannel*/
  in = g_io_channel_unix_new(rsock);

  /*添加到主循环事件*/
  g_io_add_watch(in,
                 G_IO_IN | G_IO_OUT | G_IO_HUP,
                 (GIOFunc)deal,
                 NULL);
  
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  
  return 0;
}


