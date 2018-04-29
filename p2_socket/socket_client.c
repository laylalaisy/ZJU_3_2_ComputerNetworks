/*
 * socket_client.c
 *
 * Created on: April 2, 2018
 * Author: Layla Lai
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>     // func: socket;
#include <netinet/in.h>     // func: sockaddr_in;
#include <string.h>         // func: memset;
#include <errno.h>          // func: errno;
#include <arpa/inet.h>      // func: inet_aton;
#include <zconf.h>          // func: close;

#define BUFFER_LENGTH 1024
#define SOCK_PORT 5330      // port

int main(int argc, char **argv) {
    int sockfd; // file description of socket
    int tempfd; // temp file description
    struct sockaddr_in s_addr;      // socket address
    char data_send[BUFFER_LENGTH];  // data send
    char data_recv[BUFFER_LENGTH];  // data receive
    fd_set rfds;    // fd_set
    int maxfd;      // 需要检查的文件描述字个数
    struct timeval tv;  // timeout
    int retval;
    int recvlen, sendlen;

    // get file description of socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket");
        exit(errno);
    }
    else{
        printf("socket create success!\n");
    }

    // set the attr of structure socket address
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;    // ipv4
    s_addr.sin_port = htons(SOCK_PORT); // port
    if(inet_aton(argv[1], (struct in_addr *)&s_addr.sin_addr.s_addr) == 0){
        perror(argv[1]);
        exit(errno);
    }
    else{
        printf("socket address create success!\n");
    }

    // connect to server
    tempfd = connect(sockfd, (struct sockaddr*)&s_addr, sizeof(struct sockaddr));
    if( tempfd == -1){
        perror("connect");
        exit(errno);
    }
    else{
        printf("connect to server success!\n");
    }

    // communicate
    while(1){
        // set rfds
        FD_ZERO(&rfds);         // initialize rfds
        FD_SET(0, &rfds);       // add 0 to set
        maxfd = 0;              // set 0 as maxfd
        FD_SET(sockfd, &rfds);  // add sockfd to set
        if(maxfd < sockfd){     // set maxfd
            maxfd = sockfd;
        }

        // timeout
        tv.tv_sec = 6;  // second
        tv.tv_usec = 0;

        // select, return the number of available files
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);

        if(retval == -1){
            perror("select error, client exit!\n");
            break;
        }
        else if(retval == 0){
            continue;
        }
        else{
            // user input message and send it
            if(FD_ISSET(0, &rfds)){
                bzero(data_send, BUFFER_LENGTH);        // initialize
                fgets(data_send, BUFFER_LENGTH, stdin); // input

                if(!strncasecmp(data_send, "quit", 4)){
                    printf("client require to quit!\n");
                    break;
                }

                sendlen = send(sockfd, data_send, strlen(data_send), 0);
                if( sendlen > 0){
                    printf("data send success!\n");
                }
                else{
                    perror("data send: Client has NOT sent your message!\n");
                    break;
                }
            }

            // receive message
            if(FD_ISSET(sockfd, &rfds)){
                bzero(data_recv, BUFFER_LENGTH);
                recvlen = recv(sockfd, data_recv, BUFFER_LENGTH, 0);
                if( recvlen > 0){
                    printf("data receive:\n%s\n", data_recv);
                }
                else if(recvlen == 0){
                    perror("data receive: Server has quit and stopped chatting!\n");
                }
                else{
                    perror("data receive: Server has NOT sent message successfully!\n");
                }
            }
        }
    }

    // close socket
    close(sockfd);

    return 0;
}
