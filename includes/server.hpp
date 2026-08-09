#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include "httpRequest.hpp"
#include "client.hpp"
#include "config.hpp"
#include "configParser.hpp"
#include "server.hpp"
#include <sys/epoll.h>

#define RECV_BUFFER_SIZE 1024
#define MAX_CLIENTS 1024

class Server {

	public:
	std::vector<int> listeningSockets;
	std::map<int, Client> clients;

	int epfd; //needs to be initilazied by the constructor
	struct epoll_event events[100];

	Server(const std::vector<ServerConfig>& servers); //Initializer

	void eventLoop(void);

	void createListeningSocket(int port); // the function itself will do listeningSockets.push_back(fd);
	void initializeEpoll(void);

	bool isListeningSocket(int fd);

	void acceptNewClient(int listening_fd);
	void removeClient(int fd);

	// void handleClientEvent(int fd);
	void receiveData(int fd);
	bool requestComplete(int fd);
	// void processRequest(int fd);

	void server_cleanup(void);

};

// if (isListeningSocket(fd))
//     acceptNewClient(fd);
// else
//     handleClientEvent(fd);

#endif