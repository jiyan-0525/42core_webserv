#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include "httpRequest.hpp"
#include "client.hpp"

class server {

	private:
	std::vector<int> listeningSockets;
	std::map<int, Client> clients;

	int epfd; //needs to be initilazied by the constructor
	struct epoll_event events[100];

	void createListeningSocket(int port); // the function itself will do listeningSockets.push_back(fd);
	void initializeEpoll(void);

	public:
	server(const std::vector<ServerConfig>& servers); //Initializer

	bool isListeningSocket(int fd);

	void acceptNewClient(int listening_fd);
	void addnewClient(int fd);
	void removeClient(int fd);

	void eventLoop(void);

	void handleClientEvent(int fd);
	void receiveData(int fd);
	bool requestComplete(int fd);
	void processRequest(int fd);

	void server_cleanup(void);

};

// if (isListeningSocket(fd))
//     acceptNewClient(fd);
// else
//     handleClientEvent(fd);

#endif