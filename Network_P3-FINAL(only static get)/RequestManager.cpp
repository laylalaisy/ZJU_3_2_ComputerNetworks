#include "RequestManager.hpp"
#include "csapp.h"

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <pthread.h>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_LINE 	8192
#define MAX_BUF  	8192

/* Static functions */
void* do_request(void * arg);
static void HandlePost(int fd, char* uri, rio_t * rio);
static void HandleGet(int fd, char* uri, rio_t * rio);
static void read_requesthdrs(rio_t *rp);
static void parse_uri(char *uri, char *filename);
void get_filetype(char *filename, char *filetype);
void get_response(int fd, char *filename, int filesize);
static void post_response(int connfd, bool log);
void iClient_error(int connfd, char* err, char* shortmsg, char* longmsg);

/* Constructor Function*/
RequestManager::RequestManager(int connfd) : connfd(connfd)
{ 
} 

/* Run Function*/
void RequestManager::Run()
{
	pthread_t tid;
	/* create pthread */
	if (pthread_create(&tid, NULL, do_request, (void *)(&connfd)) == -1)
	{
		perror("Error creating new thread");
		return;
	}
	/* wait */
	pthread_join(tid, NULL);	
}

/* do_request Function: handle request from client */
void* do_request(void * arg)
{
	rio_t rio;					// ref csapp
	int connfd = *(int *)arg;	// connection file descriptor
	char buf[MAX_LINE], method[MAX_LINE], uri[MAX_LINE], version[MAX_LINE];
	
	// read request
	rio_readinitb(&rio, connfd);
	rio_readlineb(&rio, buf, MAX_LINE);

	// parse request
	sscanf(buf, "%s %s %s", method, uri, version);	

	// output information
	std::cout << method << std::endl;
	std::cout << uri << std::endl;
	std::cout << version << std::endl;

	if (!strcasecmp(method, "GET"))			// GET
	{
		HandleGet(connfd, uri, &rio);
		pthread_exit(NULL);
	}
	else if (!strcasecmp(method, "POST"))	// POST
	{
		HandlePost(connfd, uri, &rio);
		pthread_exit(NULL);
	}
	else									// ERROR
	{
		iClient_error(connfd, "501", "Not implemented", "This kind of request is not supported in this server");
		pthread_exit(NULL);		
	
	}
}

/* HandleGet Function: handle GET request from client */
void HandleGet(int connfd, char* uri, rio_t* rio)
{
	struct stat sbuf;
	char buf[MAX_LINE], version[MAX_LINE];
	char filename[MAX_LINE];

	// read request 
	read_requesthdrs(rio);

	// parse URI from GET request
	parse_uri(uri, filename);

	// check if file exist
	if(stat(filename, &sbuf) < 0){
		iClient_error(connfd, "404", "Not found", "Cannot find this file");
		return;
	}
	// static content：check if is readable
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
		iClient_error(connfd, "403", "Forbidden", "Cannot read the file");
		return;
	}

	get_response(connfd, filename, sbuf.st_size);
}

/* get_response: send response header from server to client */ 
/* refer to csapp */
void get_response(int fd, char *filename, int filesize){
	int srcfd;
	char *srcp, filetype[MAX_LINE], buf[MAX_LINE];

	// get filetype
	get_filetype(filename, filetype);		

	// send response head
	sprintf(buf, "HTTP/1.0 200 OK\r\n");	
	sprintf(buf, "%sServer: Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	rio_writen(fd, buf, strlen(buf));		

	// send response body to client 
	srcfd = Open(filename, O_RDONLY, 0);	// open
	srcp = (char *)Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);	// mmap: 将被请求文件映射到一个虚拟存储器空间，
	                                                            		// 调用mmap将文件srcfd的前filesize个字节映射到
																		// 一个从地址srcp开始的私有只读存储器区域
	rio_writen(fd, srcp, filesize);			// 执行到客户端的实际文件传动，拷贝从srcp位置开始的filesize个字节到客户端的已连接描述符

	Close(srcfd);						
	Munmap(srcp, filesize);					// 释放了映射的虚拟存储器区域，避免潜在的存储器泄露
}

/* read_requesthdrs： only read requst head  */
static void read_requesthdrs(rio_t *rp){
	char buf[MAX_LINE];

	rio_readlineb(rp, buf, MAX_LINE);	// read request to buffer
	while(strcmp(buf, "\r\n")){			// end of request head
		rio_readlineb(rp, buf, MAX_LINE);
		printf("%s", buf);				// print request head
	}
	return;
}

/* parse_uri:
	假设静态内容的主目录就是当前目录；可执行文件的主目录是./cgi-bin/，任何包含该字符串的URI都认为是对动态内容的请求
	将URI解析为一个文件名和一个可选的CGI参数字符串
*/
static void parse_uri(char *uri, char *filename){
	char *ptr;

	strcpy(filename, "./dir");					// begin convert
	strcat(filename, uri);					// end convert
	if(uri[strlen(uri)-1] == '/'){			// slash check：if '/', show default file
		strcat(filename, "html/test.html");	// append default
	}		
	return;		
}

/* get_filetype:
	derive file type from file name
*/
void get_filetype(char *filename, char *filetype){
	if(strstr(filename, ".html")){			// text/html
		strcpy(filetype, "text/html");
	}
	else if(strstr(filename, ".jpg")){		// image/jpeg
		strcpy(filetype, "image/jpeg");
	}
	else{
		strcpy(filetype, "text/plain");
	}
}


void HandlePost(int connfd, char * uri, rio_t* rio)
{
	char buf[MAX_LINE]; 
	bool log = false;
	if (strcasecmp(uri, "/html/dopost") || strcasecmp(uri, "/dopost"))
	{
		iClient_error(connfd, "404",  "Not found", "This kind of resource is not available in this server");
		return;
	}
	else
	{
		int contentlength;
		rio_readlineb(rio, buf, MAX_LINE);
		std::cout << buf;

		/* Read till \r\n appear */
		while (strcmp(buf, "\r\n"))
		{
			if (strstr(buf, "Content-Length:"))
				sscanf(buf+strlen("Content-Length:"), "%d", &contentlength);
			rio_readlineb(rio, buf, MAX_LINE);
			std::cout << buf;
		} 
		std::cout << "Finish reading header" << std::endl;	
		/* Now we get the body */
		rio_readlineb(rio, buf, contentlength+1);
		/* Output the body */
		std::cout << "Body:" << buf << std::endl;
		char login[MAX_LINE], pass[MAX_LINE];		
		if (strstr(buf, "login=") && strstr(buf, "pass="))
		{
			char * p = strstr(buf, "login=");
			int li = 0, pi = 0;
			while (*p++ != '=');
			while (*p != '&')
			{
				login[li++] = *p++;
			}
			login[li] = 0;
			while (*p++ != '=');
			while (*p)
			{
				pass[pi++] = *p++;
			}
			pass[pi] = 0;
		}
		/* Login status check */
		std::cout << "login = " << login << "pass = " << pass << std::endl;
		if (!strcmp(login, "3150101224") && !strcmp(pass, "1224"))
			log = true;
		else
			log = false;
		
		/* Return status */
		post_response(connfd, log);	
	}
}

void post_response(int connfd, bool log)
{
	char buf[MAX_LINE];
	char * successmsg = "<html><body>Success</body></html>";
	char * failmsg = "<html><body>Fail</body></html>";
	
	sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
	sprintf(buf, "%sServer: Server ",buf); 
	sprintf(buf, "%sContent-length: %d\r\n", buf, log ? strlen(successmsg) : strlen(failmsg) ); 
	sprintf(buf, "%sContent-type: text/html\r\n\r\n", buf);
	 
	/* Log in successful */
	
	rio_writen(connfd, buf, strlen(buf));
	if (log)
	rio_writen(connfd, successmsg, strlen(successmsg));
	else
	rio_writen(connfd, failmsg, strlen(failmsg));

}

void iClient_error(int connfd, char* err, char* shortmsg, char* longmsg)
{
	char buf[MAX_LINE], body[MAX_LINE];
	sprintf(body, ""); 
	sprintf(body, "%s%s: %s\r\n", body, err, shortmsg); 
	sprintf(body, "%s%s\r\n", body, longmsg); 
	sprintf(body, "%sError msg from Web Server\r\n", body);
	
	sprintf(buf, "HTTP/1.0 %s %s\r\n", err, shortmsg);  
	rio_writen(connfd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n"); 
	rio_writen(connfd, buf, strlen(buf)); 
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); 
	rio_writen(connfd, buf, strlen(buf));
 
	rio_writen(connfd, body, strlen(body)); 	
}

