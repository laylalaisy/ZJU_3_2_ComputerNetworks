#include "NetworkManager.hpp"
#include "RequestManager.hpp"

#include <iostream>

int main(int argc, char** argv)
{
	NetworkManager nm;
	int connfd;
	
	/* Start listening on port 1224 */
	nm.Listen(1224);
	std::cout << "Web Server started.. Waiting for request" << std::endl;	
	
	/* Infinity loop on accepting request and handling */
	while (1)
	{
		connfd = nm.Accept();
		RequestManager rm(connfd);
		std::cout << "Connection Established, socket id = " << connfd << std::endl;
		rm.Run();
		nm.Close();
	}

	return 0;
}
