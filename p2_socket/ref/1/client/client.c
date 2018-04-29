#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <zconf.h>

#define PORT 6666
#define BUFLEN 1024

int main(int argc, char **argv){
    int sockfd;                     // socket
    struct sockaddr_in s_addr;      // server's IP
    fd_set rfds;                    //一long类型的数组，与一打开的文件句柄建立联系
    int maxfd;                      // 参数maxfd是需要监视的最大的文件描述符值+1
    struct timeval tv;              // timeout
    int retval;                     // 对应位仍然为1的fd的总数
    char buf[BUFLEN];               // buffer
    socklen_t len;                  // socklen type

    /* Create Socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){   // create a new socket
        perror("socket");           // print error
        exit(errno);                // exit
    }
    else{
        printf("socket create success!\n");                 // print success
    }

    /* Set Server's IP */
    memset(&s_addr, 0, sizeof(s_addr));    // initialize s_address
    s_addr.sin_family = AF_INET;           // domain
    s_addr.sin_port = htons(PORT);         // 整型变量从主机字节顺序转变成网络字节顺序
    // 将一个字符串IP地址转换为一个32位的网络序列IP地址
    if(inet_aton(argv[1], (struct in_addr *)&s_addr.sin_addr.s_addr) == 0){ 
        perror(argv[1]);                   // print error
        exit(errno);                       // exit
    }

    /* Connect To Server */
    if(connect(sockfd, (struct sockaddr*)&s_addr, sizeof(struct sockaddr)) == -1){
        perror("connect");
        exit(errno);
    }
    else{
        printf("connect success!\n!");
    }

    while(1){
        // rfds
        FD_ZERO(&rfds);         // clean rfds
        FD_SET(0, &rfds);       // set rfds
        FD_SET(sockfd, &rfds);  // set sockfd to rfds

        // maxfd
        maxfd = 0;
        if(maxfd < sockfd){
            maxfd = sockfd;     // set maxfd
        }

        // timeout
        tv.tv_sec = 6;          // second
        tv.tv_usec = 0;         // microsecond

        // select
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);

        if(retval == -1){       // recieve error
            printf("Select ERROR!\n");
            break;
        }
        else if(retval == 0){   // wait
            printf("Waiting...\n");
            continue;
        }
        else{                   // recieve success
            /* Server Send A Message */
            if(FD_ISSET(sockfd, &rfds)){
                /* Receive Message */
                bzero(buf, BUFLEN);                 // 置字节字符串前n个字节为零且包括‘\0’
                len = recv(sockfd, buf, BUFLEN, 0); // 返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；
                                                    // 如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
                if(len > 0){        // receive successfully
                    printf("Server's Message: %s\n", buf);
                }
                else if(len == 0){  // net break
                    printf("Server is stop!\n");
                    break;
                }
                else{               // receive failed
                    printf("Receive failed!\n");
                    break;
                }
            }

            /* Client Input a Message */
            if(FD_ISSET(0, &rfds)){
                /* Send Message */
                bzero(buf, BUFLEN);                 // 置字节字符串前n个字节为零且包括‘\0’
                fgets(buf, BUFLEN, stdin);          // get input and store in buffer

                if(!strncasecmp(buf, "quit", 4)){   // strncasecmp()用来比较参数s1和s2字符串前n个字符，比较时会自动忽略大小写的差异
                    printf("Client want to stop!\n");
                    break;
                }

                len = send(sockfd, buf, strlen(buf), 0);    // send client's input

                if(len > 0){
                    printf("\tSend successful: %s\n", buf);
                }
                else{
                    printf("Send failed!\n");
                    break;
                }
            }
        }

    }

    /* close socket */
    close(sockfd);

    return 0;
}