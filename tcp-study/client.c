#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

int
main(int argc, char *argv[]){
  if(argc != 3){
    printf("Usage ./server [ip] [port]\n");
    return 1;
  }
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0){
    perror("socket");
    return 1;
  }

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(atoi(argv[2]));
  int ret = connect(fd, (sockaddr *)&server_addr, sizeof(server_addr));
  if(ret < 0){
    perror("bind");
    return 1;
  }

  while(1){
    char buf[1024] = {0};
    /* 从标准输入读取字符串 */
    ssize_t read_size = read(0, buf, sizeof(buf) -1);
    if(read_size < 0){
      perror("read");
      return 1;
    }
    if(read_size == 0){
      printf("read done!\n");
      return 0;
    }
    buf[read_size] = '\0';
    write(fd, buf, strlen(buf));
    //c)尝试从服务器读取响应数据
    char buf_resp[1024] = {0};
    read_size = read(fd, buf_resp, sizeof(buf_resp) - 1);
    if(read_size < 0){
        perror("read");
        return 1;
    }
    if(read_size == 0){
        printf("server close socket!\n");
        return 0;
    }
    buf_resp[read_size] = '\0';
    //d)把响应结果打印到标准输出上
    printf("server resp: %s\n", buf_resp);
  }
  return 0;
}