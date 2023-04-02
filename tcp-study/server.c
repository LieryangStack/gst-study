#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

void 
ProcessConnect(int new_sock){

  while(1){
    /*从客户端读取数据*/
    char buf[1024] = {0};
    ssize_t read_size = read(new_sock, buf, sizeof(buf) - 1);
    printf("read_size = %ld\n", read_size);
    if(read_size < 0){
      perror("read");
      continue;
    }
    /*没有读取到数据，说明客户端已关闭了连接*/
    if(read_size == 0){
      printf("[client %d] disconnect!\n", new_sock);
      close(new_sock);
      return;
    }
    buf[read_size] = '\0';
    printf("[client %d] %s\n", new_sock, buf);
    /*把响应写回到客户端*/
    write(new_sock, buf, strlen(buf));
  }
}

int
main(int argc, char *argv[]){
  if(argc != 3){
    printf("Uage ./server [ip] [port]\n");
    return 1;
  }
  /* 1.创建socket,AF_INET表示IPv4协议，SOCK_STREAM表示字节流传输协议*/
  int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_sock < 0){
    perror("socket");
    return 1;
  }
  /* 2.绑定ip地址和端口 */
  sockaddr_in server;
  server.sin_family = AF_INET;
  /*inet_addr将ip地址从点数转换成32位无符号长整形*/
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(atoi(argv[2]));
  int ret = bind(listen_sock, (sockaddr *)&server, sizeof(server));
  if(ret < 0){
    perror("bind");
    return 1;
  }
  /* 3.使用listen运行服务器被客户端连接 */
  ret = listen(listen_sock, 5);
  if(ret < 0){
    perror("listen");
    return 1;
  }
  /* 4.服务器初始化完成，进入事件循环 */
  printf("Server Init OK!\n");
  while(1){
    sockaddr_in peer;
    socklen_t len = sizeof(peer);
    /*系统会阻塞在这儿等待客户端连接*/
    int new_sock = accept(listen_sock, (sockaddr *)&peer, &len);
    /* 输出客户端IP地址
     * inet_ntoa 无符号32位IP转换成点型
     */    
    printf("Client Addr:%s，Port:%d Linked******\n", inet_ntoa(peer.sin_addr), peer.sin_port);

    if(new_sock < 0){
      perror("accept");
      continue;
    }
    printf("[client %d]connect!\n", new_sock);
    ProcessConnect(new_sock);
  }

  return 0;
}