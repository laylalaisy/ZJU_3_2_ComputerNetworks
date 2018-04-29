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
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
static void post_response(int connfd, bool log);
static void HandlePost(int fd, char* uri, rio_t * rio);
static void HandleGet(int fd, char* uri, rio_t * rio);
void* do_request(void * arg);
void iClient_error(int connfd, char* err, char* shortmsg, char* longmsg);


RequestManager::RequestManager(int connfd) : connfd(connfd)
{ 
} 

void RequestManager::Run()
{
	pthread_t tid;
	if (pthread_create(&tid, NULL, do_request, (void *)(&connfd)) == -1)
	{
		perror("Error creating new thread");
		return;
	}
	pthread_join(tid, NULL);	
}

void* do_request(void * arg)
{
	char buf[MAX_LINE], method[MAX_LINE], uri[MAX_LINE], version[MAX_LINE];
	rio_t rio;
	int connfd = *(int *)arg;
	rio_readinitb(&rio, connfd);
	rio_readlineb(&rio, buf, MAX_LINE);
	sscanf(buf, "%s %s %s", method, uri, version);

	/* Debug */
	std::cout << method << std::endl;
	std::cout << uri << std::endl;
	std::cout << version << std::endl;					
	if (!strcasecmp(method, "GET"))
	{
		HandleGet(connfd, uri, &rio);
		pthread_exit(NULL);
	}
	else if (!strcasecmp(method, "POST"))
	{
		HandlePost(connfd, uri, &rio);
		pthread_exit(NULL);
	}
	else
	{
		iClient_error(connfd, "501", "Not implemented", "This kind of request is not supported in this server");
		pthread_exit(NULL);		
	
	}
}

/* read_requesthdrs：
	不使用请求报头中的任何信息，仅读取报头
*/
void read_requesthdrs(rio_t *rp){
	char buf[MAX_LINE];

	rio_readlineb(rp, buf, MAX_LINE);
	while(strcmp(buf, "\r\n")){	//	 终止请求包头的空文本行是由回车和换行符组成的
		rio_readlineb(rp, buf, MAX_LINE);
		printf("%s", buf);
	}
	return;
}

/* parse_uri:
	假设静态内容的主目录就是当前目录；可执行文件的主目录是./cgi-bin/，任何包含该字符串的URI都认为是对动态内容的请求
	将URI解析为一个文件名和一个可选的CGI参数字符串
*/
int parse_uri(char *uri, char *filename, char *cgiargs){
	char *ptr;

	if(!strstr(uri, "cgi-bin")){	// static content: 清楚CGI参数串，然后将URI转换为一个相对的unix路径名
		strcpy(cgiargs, "");		// clearcgi
		strcpy(filename, ".");		// begin convert1
		strcat(filename, uri);		// end convert1
		if(uri[strlen(uri)-1] == '/'){		// slash check：如果用“/”结尾，把默认的文件名加在后面
			strcat(filename, "home.html");	// append default
		}		
		return 1;		
	}
	else{							// Dynamic content：抽取所有的CGi参数，并将URI剩下的部分转换为一个相应的unix文件名
		ptr = index(uri, '?');
		if(ptr){
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}
		else{
			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}

/* get_filetype:
	derive file type from file name
*/
void get_filetype(char *filename, char *filetype){
	if(strstr(filename, ".html")){
		strcpy(filetype, "text/html");
	}
	else if(strstr(filename, ".gif")){
		strcpy(filetype, "image/gif");
	}
	else if(strstr(filename, ".jpg")){
		strcpy(filetype, "image/jpeg");
	}
	else{
		strcpy(filetype, "text/plain");
	}
}

/* serve_static: 
	发送一个HTTP响应，主体包含一个本地文件的内容
*/ 
void serve_static(int fd, char *filename, int filesize){
	int srcfd;
	char *srcp, filetype[MAX_LINE], buf[MAX_LINE];

	// send response headers to client
	get_filetype(filename, filetype);		// get filetype

	sprintf(buf, "HTTP/1.0 200 OK\r\n");	// begin serve
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	rio_writen(fd, buf, strlen(buf));		// end serve

	// send response body to client
	srcfd = Open(filename, O_RDONLY, 0);	// open
	srcp = (char *)Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);	// mmap: 将被请求文件映射到一个虚拟存储器空间，
	                                                            		// 调用mmap将文件srcfd的前filesize个字节映射到一个从地址srcp开始的私有只读存储器区域
	Close(srcfd);
	rio_writen(fd, srcp, filesize);		// 执行到客户端的实际文件传动，拷贝从srcp位置开始的filesize个字节到客户端的已连接描述符
	Munmap(srcp, filesize);				// 释放了映射的虚拟存储器区域，避免潜在的存储器泄露
}

/* serve_dynamic:
	派生一个子进程并在子进程的上下文中运行一个CGI程序来提供各种类型的动态内容
*/
void serve_dynamic(int fd, char *filename, char *cgiargs){
	char buf[MAX_LINE], *emptylist[] = { NULL };

	// return first part of HTTP response
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	rio_writen(fd, buf, strlen(buf));

	if(Fork() == 0){	// child
		// real server sould set all CGI vars here
		setenv("QUERY_STRING", cgiargs, 1);		// setenv
		Dup2(fd, STDOUT_FILENO);				// redirect stdout to client, dup2
		Execve(filename, emptylist, environ);	// run CGI program, execve
	}
	Wait(NULL);			// parent waits for and reaps chile
}


void HandleGet(int connfd, char* uri, rio_t* rio)
{
	int isStatic;
	struct stat sbuf;
	char buf[MAX_LINE], version[MAX_LINE];
	char filename[MAX_LINE], cgiargs[MAX_LINE];

	read_requesthdrs(rio);
	// parse URI from GET request
	isStatic = parse_uri(uri, filename, cgiargs);
	if(stat(filename, &sbuf) < 0){
		iClient_error(connfd, "404", "Not found", "Tiny could not find this file");
		return;
	}

	if(isStatic){		// static content：静态内容验证是否为普通文件，有读权限
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			iClient_error(connfd, "403", "Forbidden", "Tiny could not read the file");
			return;
		}
		serve_static(connfd, filename, sbuf.st_size);
	}
	else{				// dynamic sontent：动态内容，验证是否为可执行文件，如果是，就提供动态内容
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
			iClient_error(connfd, "403", "Forbidden", "Tiny could not run the CGI program");
			return;
		}
		serve_dynamic(connfd, filename, cgiargs);
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

