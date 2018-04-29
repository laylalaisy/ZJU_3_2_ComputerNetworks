#ifndef REQUESTMANAGER_HPP
#define REQUESTMANAGER_HPP

class RequestManager {
public:
	void Run();
	RequestManager(int connfd);
private:
	int connfd;
};


#endif
