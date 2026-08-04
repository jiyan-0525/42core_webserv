#include "../../includes/server.hpp"
#include "../../includes/client.hpp"
#include "../../includes/httpRequest.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <sys/epoll.h>
#include <fcntl.h>

#include "httpResponse.hpp"
#include "requestHandler.hpp"
#include "config.hpp"


server::server(const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        createListeningSocket(servers[i].port);
    }
	initializeEpoll();
}

void server::createListeningSocket(int port) //the function itself will do listeningSockets.push_back(fd);
{
	std::string s_port = std::to_string(port);

    int server_fd;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // Allow immediate reuse of the port (prevents "Address already in use" errors)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct addrinfo request_form;
    struct addrinfo* result;
    
    std::memset(&request_form, 0, sizeof(request_form)); // Zero out memory old-school style
    request_form.ai_family = AF_INET;             // IPv4
    request_form.ai_socktype = SOCK_STREAM;       // TCP
    request_form.ai_flags = AI_PASSIVE;           // Bind to all available interfaces (0.0.0.0)

    char *ptr = NULL;

    if (getaddrinfo(ptr, s_port.c_str(), &request_form, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        close(server_fd);
        return;
    }

    // 3. Bind the socket to the port
    if (bind(server_fd, result->ai_addr, result->ai_addrlen) < 0)
    {
        perror("In bind");
        close(server_fd);
        return;
    }
    freeaddrinfo(result);

    // 4. Start listening for incoming connections (like Chrome)
    //int listen(int socket, int backlog);
    //The second parameter, backlog, defines the maximum number of pending connections that can be queued up before connections are refused.
    if (listen(server_fd, SOMAXCONN) < 0) // SOMAXCONN maximum that the Kernel can do 
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is running... Open Chrome and go to http://localhost:8080\n";

	listeningSockets.push_back(server_fd);
}

void server::initializeEpoll(void)
{
    for (size_t i = 0; i < listeningSockets.size(); i++)
    {
        struct epoll_event server_event = {};

        server_event.events = EPOLLIN;
        server_event.data.fd = listeningSockets[i];

        epoll_ctl(epfd, EPOLL_CTL_ADD, listeningSockets[i], &server_event);
    }
}