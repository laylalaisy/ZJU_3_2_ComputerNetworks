#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>

#define BUFLEN 1024
#define PORT 6666
#define LISTNUM 20      // 参数指定队列中最多可容纳的等待接受的传入连接数


int main(int argc, char **argv){
    int sockfd, newfd;                      // socket
    struct sockaddr_in s_addr, c_addr;      // server's and client's address
    char buf[BUFLEN];                       // buffer
    socklen_t len;                          // socket length
    fd_set rfds;
    int maxfd;
    struct timeval tv;
    int retval;

    /* Create Socket */
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0) == -1)){
        perror("socket");
        exit(errno);
    }
    else{
        printf("socket create success!\n");
    }

    /* Set server's IP */
    memset(&s_addr, 0, sizeof(s_addr));     // initialize s_addr
    s_addr.sin_family = AF_INET;            // domain, set s_addr.sin_family
    s_addr.sin_port  = htons(PORT);         // 整型变量从主机字节顺序转变成网络字节顺序
    s_addr.sin_addr.s_addr = htons(INADDR_ANY); // set server's address

    /* Bind Address and Port on Socket */
    if((bind(sockfd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))) == -1){
        perror("bind");
        exit(errno);
    }
    else{
        printf("bind success!\n");
    }

    /* Listen */
    if(listen(sockfd, LISTNUM) == -1){
        perror("listen");
        exit(errno);
    }
    else{
        printf("the server is listening!\n");
    }

    /* Start to chat */
    while(1){
        printf("*****************START TO CHAT***************\n");
        len = sizeof(struct sockaddr);

        /* Accept Socket */
        if((newfd = accept(sockfd, (struct sockaddr*) &c_addr, &len)) == -1){
            perror("accept");
            exit(errno);
        }
        else{
            // output current client's address and port
            printf("Current client is: %s: %d\n", inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));
        }

        while(1){
            // rfds
            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
            FD_SET(newfd, &rfds);

            // maxfd
            maxfd = 0;
            if(maxfd < newfd){
                maxfd = newfd;
            }

            // tv
            tv.tv_sec = 6;
            tv.tv_usec = 0;

            // Waiting
            retval = select(maxfd+1, &rfds, NULL, NULL, &tv);

            if(retval == -1){   // fail
                printf("select failed, exit!\n");
            }
            else if(retval == 0){
                printf("Waiting...\n");
            }
            else{
                /* Accept from client */
                if(FD_ISSET(newfd, &rfds)){
                    /* Receive Message */
                    memset(buf, 0, sizeof(buf));        // initialize buffer
                    len = recv(newfd, buf, BUFLEN, 0);
                    if(len > 0){
                        printf("Client's Message: %s\n", buf);
                    }
                    else if(len == 0){
                        printf("Client quit!\n");
                        break;
                    }
                    else{
                        printf("Receive message failed!\n");
                        break;
                    }
                }

                /* Server input message */
                if(FD_ISSET(0, &rfds)){
                    /* Server send message */
                    memset(buf, 0, sizeof(buf));        // initialize buffer
                    fgets(buf, BUFLEN, stdin);          // get input and store in buffer
                    if(!strncasecmp(buf, "quit", 4)){
                        printf(" Server asks to stop chatting!\n");
                        break;
                    }
                    else{
                        len = send(newfd, buf, strlen(buf), 0);
                        if(len > 0){
                            printf("\tSend success: %s\n", buf);
                        }
                        else{
                            printf("Send failed!\n");
                            break;
                        }
                    }
                }
            }
            /* close newfd */
            close(newfd);

            /* whether quit server */
            printf("Do you want to quit server: y->YES; n->NO?");
            bzero(buf, BUFLEN);
            fgets(buf, BUFLEN, stdin);
            if(!strncasecmp(buf, "y",1)){
                printf("Server quit!\n");
                break;
            }
        }
    }

    /* close socket */
    close(sockfd);
    return 0;
}