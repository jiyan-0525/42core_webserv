#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include "httpRequest.hpp"

struct Client
{
	int fd;
	std::string buffer;
	bool request_complete;
	
	HttpRequest request;

	bool keep_alive;

	unsigned long total_bytesSent;
	
	Client(int socket_fd)
	{
		fd = socket_fd;
		buffer = "";
		request_complete = false;
		keep_alive = true;
		total_bytesSent = 0;
	}
};

#endif
