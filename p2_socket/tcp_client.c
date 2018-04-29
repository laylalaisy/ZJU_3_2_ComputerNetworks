#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <pthread.h>

#define ADDR_LENGTH 40
#define PORT_LENGTH 6

int Prompting(int);
void *ListenResponse();
int Cliconnect();
int Cliconnect_withip(char *ip_addr, char *ip_port, pthread_t *tid);
int Cliclose(int network_socket, pthread_t tid);
int Cligettime(int network_socket);
int Cligetname(int network_socket);
int Cligetserver(int network_socket);
int Clisendserver(int network_socket);

int Cligetserver(int network_socket)
{
	if (network_socket == -1)
	{
		perror("Socket not available");
		return 1;
	}
	int rv;
	rv = send(network_socket, "list", sizeof("list"), 0);
	if (rv == -1)
		return 1;
	else
		return 0;
}

int Clisendserver(int network_socket)
{
	if (network_socket == -1)
	{
		perror("Socket not available");
		return 1;
	}
	char msg[246];
	char msgno[256];
	int rv, no;
	printf("Please input the number of the client:\n");
	scanf("%d", &no);
	printf("Please input the message you want to send:\n");
	scanf("%s", msg);	
	//itoa(no, msgno, 10);
	sprintf(msgno, "send:%d,", no);
	strcat(msgno, msg); 
	rv = send(network_socket,msgno, sizeof(msgno), 0); 
	if (rv == -1)
		return 1;
	else
		return 0;
}

int Cligetname(int network_socket)
{
	if (network_socket == -1)
	{
		perror("Socket not available");
		return 1;
	}
	int rv;
	rv = send(network_socket, "name", sizeof("name"), 0);
	if (rv == -1)
		return 1;
	else
		return 0;
}

int Cligettime(int network_socket)
{
	if (network_socket == -1)
	{
		perror("Socket not available");
		return 1;
	}
	int rv;
	rv = send(network_socket, "time", sizeof("time"), 0);
	if (rv == -1)
		return 1;
	else
		return 0;
}

int Cliclose(int network_socket, pthread_t tid)
{
	int rv;
	rv = send(network_socket, "quit", sizeof("quit"),0);
	close(network_socket);
	return 0;
}


int Cliconnect(pthread_t *tid)
{
	char ip_addr[ADDR_LENGTH], ip_port[PORT_LENGTH];
	printf("Please enter ip:\n");
	scanf("%s", ip_addr);
	printf("Please enter port:\n");
	scanf("%s", ip_port);
	int retval = Cliconnect_withip(ip_addr, ip_port, tid);
	return retval;
}

void *ListenResponse(void * arg)
{
	int network_socket;
	char server_response[256];
	network_socket = (int)(*(int*)arg);
	memset(server_response, 0, sizeof(server_response));
	while (1)
	{
		int recv_ret = recv(network_socket,&server_response,sizeof(server_response), 0);
		if (recv_ret > 0)
		{
			printf("[Data received] ");
			printf("%s\n", server_response);
			memset(server_response, 0, sizeof(server_response));
		} 
		else if (recv_ret < 0)
		{
			perror("Invalid socket\n");
			pthread_exit(NULL);
		}
		else if (recv_ret == 0)
		{
			printf("Socket closed, exiting current thread\n");
			pthread_exit(NULL);
		}
	}
	pthread_exit(NULL);
}
int Cliconnect_withip(char *ip_addr, char *ip_port, pthread_t *tid)
{
	// socket
	int network_socket;
	// ip port
	int port;
	// char buffer
	char server_response[256];
	struct hostent *server;
	struct sockaddr_in serv_addr;
	
	network_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (network_socket < 0)
	{
		perror("Error opening socket");
		exit(0);
	}
	
	// convert port to an integer
	port = atoi(ip_port);
	server = gethostbyname(ip_addr);
	// if not exist
	if (server == NULL) 
	{
		perror("NO such host");
		exit(0);
	}
	// filling the server's address.
	bzero( (char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(ip_addr, (struct in_addr *)&serv_addr.sin_addr.s_addr);
//	serv_addr.sin_addr.s_addr = inet_addr(server->h_addr);
//	bcopy((char *)server->h_addr,
//			(char *)&serv_addr.sin_addr /*sin_addr.s_addr*/,
//			server->h_length);
	if (connect(network_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error connecting");
		exit(0);
	}	
	int rc;
	rc = pthread_create(tid, NULL, &ListenResponse, &network_socket);
	if (rc)
	{
		perror("Error when creating new thread");
		exit(0);
	}
	else // parent process
	{
		printf("Connection established\n");	
		return network_socket;
	}
	return network_socket;
}
	


int Prompting(int inst)
{
	int command;
	printf("Welcome to network experiment2!\n");
	printf("1. Connect to specific IP and port.\n");
	if(inst != 0) {
	printf("2. Disconnect.\n");
	printf("3. Get the time.\n");
	printf("4. Get the name.\n");
	printf("5. Get the server list.\n");
	printf("6. Send the message.\n");}
	printf("7. Exit.\n");
	if (inst != 1) {
	printf("Please enter the command:\n");
	scanf("%d", &command); }
	while (command < 1 || command > 7)
	{
		printf("Wrong instruction, please try again");
		scanf("%d", &command);
	}
	return command;
} 

int main()
{
	// Main loop of listing menu.
	int command;
	int socket = -1;
	pthread_t tid;
	int rv;
	while(1)
	{
		if (socket == -1)
			command = Prompting(0);	
		else
			scanf("%d", &command);
		switch( command)
		{
			case 1:	socket = Cliconnect(&tid); Prompting(1); break;	
			case 2: if (socket != -1)
						Cliclose(socket, tid);
					printf("Connection has been closed\n");
					socket = -1;
					break;
			case 3: rv = Cligettime(socket);
					if (rv != 0)
						printf("Error when get time\n");
					break;
			case 4:	rv = Cligetname(socket);
					if (rv != 0)
						printf("Error when get name\n");
					break;
			case 5: rv = Cligetserver(socket);
					if (rv != 0)
						printf("Error when get the client list");
					break;
			case 6: rv = Clisendserver(socket);
					if (rv != 0)
						printf("Error when get the client list");
					break;
			case 7: if (socket == -1)
						exit(0);
					else
					{
						Cliclose(socket, tid);
						socket = -1;
						exit(0);
					}
					break;
			default: break;
		}
	}	
//	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	// check for error with the connection
//	if (connection_status == -1) {
//		printf("There was an error making a connection to the remote socket \n\n ");
//	}	
	// receive data from the server
//	char server_response[256];
//	recv(network_socket, &server_response, sizeof(server_response), 0);
//	
//	// print out the server's response
//	printf("The server sent the data: %s\n", server_response);
//
//	// and then close the socket
//	close(network_socket);
	return 0;	
}
