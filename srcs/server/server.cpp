#include "../../includes/server.hpp"
#include "../../includes/client.hpp"
#include "../../includes/httpRequest.hpp"
#include "../../includes/httpResponse.hpp"
#include "../../includes/requestHandler.hpp"
#include "../../includes/config.hpp"
#include "../../includes/signals.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>

Server::Server(const std::vector<ServerConfig>& servers)
{
    this->serverConfigs = servers;
    for (size_t i = 0; i < servers.size(); i++)
    {
        createListeningSocket(servers[i].port, i);
    }
	initializeEpoll();
}

void Server::eventLoop(void)
{
    while (g_running)
    {
        int events_number = epoll_wait(this->epfd, this->events, 100, 100);

        if (events_number == -1)
        {
            if (!g_running || errno == EINTR)
                break ;
            std::cerr << "epoll_wait: " << strerror(errno) << std::endl;
            break ;
        }
        else if (events_number == 0)
            continue;
        
        for (int i = 0; i < events_number; i++)
        {
            int fd = this->events[i].data.fd;

            if (this->isListeningSocket(fd) == true)  //--------------Is there a new client that want to connect?------------------
            {
                    if (clients.size() < 2)
                        std::cout << "before_accepting_new_client" << std::endl;
                    this->acceptNewClient(fd);
            }
            else  //-------------Is there an event from an already connected client? //check for the readiness of I/O Input/Output
            {
                if (clients.size() < 2)
                    std::cout << "Event from an already connected client" << std::endl;
                if (events[i].events & EPOLLIN) //checking for reading
                {
                    this->receiveData(fd);
                }
                if (events[i].events & EPOLLOUT) //checking for writing
                {
                    sendResponse(fd);
                }
            }
        }
    }
    std::cout << "\nFinal clients.size() = " << clients.size() << std::endl;
    this->server_cleanup();
    return;
}

void Server::createListeningSocket(int port, size_t serverIndex) //the function itself will do listeningSockets.push_back(fd);
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

    //Start listening for incoming connections
    //The second parameter, backlog, defines the maximum number of pending connections that can be queued up before connections are refused.
    if (listen(server_fd, SOMAXCONN) < 0) // SOMAXCONN maximum that the Kernel can do 
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is running... Open Chrome and go to http://localhost:" << s_port << std::endl;

	listeningSockets.push_back(server_fd);
    this->listeningSocketToServer[server_fd] = serverIndex;
}

void Server::initializeEpoll(void)
{
    this->epfd = epoll_create1(0);

    for (size_t i = 0; i < listeningSockets.size(); i++)
    {
        struct epoll_event server_event = {};

        server_event.events = EPOLLIN;
        server_event.data.fd = listeningSockets[i];

        epoll_ctl(this->epfd, EPOLL_CTL_ADD, listeningSockets[i], &server_event);
    }
}

void Server::acceptNewClient(int listening_fd)
{
    if (clients.size() < 2)
        std::cout << "new_client_COMING from listening_socket_fd: " << listening_fd << std::endl;
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = 0;
    client_fd = accept(listening_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0)
        return;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct epoll_event client_event = {};
    client_event.events = EPOLLIN;   //Tell me when the socket is ready for reading
    client_event.data.fd = client_fd;

    int add = epoll_ctl(this->epfd, EPOLL_CTL_ADD, client_fd, &client_event); //added the new client to the epoll
    if (add == -1)
        return;
    this->clients.insert(std::make_pair(client_fd, Client(client_fd)));
    if (clients.size() < 2)
        std::cout << "New client fd: " << client_fd << " | clients.size() = " << clients.size() << std::endl;
}

bool Server::isListeningSocket(int fd)
{
    std::vector<int>::const_iterator it;
    for (it = this->listeningSockets.cbegin(); it != this->listeningSockets.cend(); it++)
    {
        if (fd == *it)
        {
            if (clients.size() < 2)
                std::cout << "Event from listenning socket number:  " << fd << std::endl;
            return (true);
        }
    }
    return false;
}

static size_t getContentLength(const std::string& buffer)
{
    size_t pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
        pos = buffer.find("content-length:");

    if (pos == std::string::npos)
        return 0;

    pos += std::string("Content-Length:").length();

    // Skip spaces
    while (pos < buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
        pos++;

    size_t end = buffer.find("\r\n", pos);
    if (end == std::string::npos)
        end = buffer.find("\n", pos);

    if (end == std::string::npos)
        return 0;

    std::string length = buffer.substr(pos, end - pos);
    return std::atoi(length.c_str());
}

bool Server::requestComplete(int fd)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return false;

    // 1. Support of \r\n\r\n (standard), also \n\n (nc/telnet)
    size_t headerEnd = it->second.buffer.find("\r\n\r\n");
    size_t delimiterLen = 4;

    if (headerEnd == std::string::npos)
    {
        headerEnd = it->second.buffer.find("\n\n");
        delimiterLen = 2;
    }

    // headers have not yet been received in full
    if (headerEnd == std::string::npos)
        return false;

    // 2. If it is GET or DELETE — after headers the request is ready
    if (it->second.buffer.find("GET") == 0 || it->second.buffer.find("DELETE") == 0 ||
        it->second.buffer.find("GET ") != std::string::npos || it->second.buffer.find("DELETE ") != std::string::npos)
    {
        return true;
    }

    // 3. If it is POST — check if we have fuul body by Content-Length
    if (it->second.buffer.find("POST") != std::string::npos)
    {
        size_t contentLength = getContentLength(it->second.buffer);
        size_t bodySize = it->second.buffer.size() - (headerEnd + delimiterLen);

        if (bodySize >= contentLength)
            return true;
    }

    return false;
}

bool Server::knownRequest(int fd)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return false;

    // we check the method only after we have at least first line of request (Request Line)
    size_t firstLineEnd = it->second.buffer.find("\r\n");
    if (firstLineEnd == std::string::npos)
        firstLineEnd = it->second.buffer.find("\n");

    // even if we haven't finished reading the first line yet — we wait to continue (return true)
    if (firstLineEnd == std::string::npos)
        return true;

    if (it->second.buffer.find("GET") != std::string::npos || 
        it->second.buffer.find("DELETE") != std::string::npos || 
        it->second.buffer.find("POST") != std::string::npos)
    {
        return true;
    }

    if (clients.size() < 2)
        std::cout << "My client received an unknown request" << std::endl;
    return false;
}

void Server::receiveData(int fd)
{
    char buffer[RECV_BUFFER_SIZE];
    ssize_t bytesRecv = 0;
                        
    bytesRecv = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRecv == 0 || bytesRecv == -1) // == 0 client closed the connection // == -1 EAGAIN/EWOULDBLOCK are values stored in errno explaining that -1 means "would block."
    {
        removeClient(fd);
        return;
    }
    else if (bytesRecv > 0)
    {
        buffer[bytesRecv] = '\0';      // Make it a C-string

        if (clients.size() < 2)
            std::cout << "bytes_received_from_client" << std::endl;
        std::map<int, Client>::iterator it = clients.find(fd);
        if (it == clients.end())
            return;
        else if (it != clients.end())
        {
            if (clients.size() < 2)
                std::cout << "filled the client struct with the HTTP request" << std::endl;
            it->second.buffer.append(buffer, bytesRecv); 
        }
        if (knownRequest(fd) == false)
        {
            const char* errorResponse = "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
            send(fd, errorResponse, strlen(errorResponse), 0);
            removeClient(fd);
            return;
        }
        if (requestComplete(fd) == true) //Once the request is complete and it is a known request, I want epoll() to tell me when the socket is ready for writing
        {
            if (clients.size() < 2)
            {
                std::cout << "RECEIVED COMPLETE REQUEST" << std::endl;
                std::cout << it->second.buffer << std::endl;
            }
            struct epoll_event event = {};
            event.events = EPOLLOUT;
            event.data.fd = fd;
            epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
        }
    }
}

void Server::sendResponse(int fd)
{
    ssize_t bytesSent = 0;
    std::map<int, Client>::iterator it = clients.find(fd);

    // --- (real request, real config) ---
    HttpRequest request;
    request.parseRequest(it->second.buffer); // Parse full accumulated request

    ServerConfig server;
    int localPort = -1;
    sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    if (getsockname(fd, (sockaddr*)&localAddr, &localLen) == 0)
        localPort = ntohs(localAddr.sin_port);

    size_t matchedIndex = 0;
    for (size_t idx = 0; idx < this->serverConfigs.size(); ++idx)
    {
        if (this->serverConfigs[idx].port == localPort)
        {
            matchedIndex = idx;
            break;
        }
    }

    if (!this->serverConfigs.empty())
        server = this->serverConfigs[matchedIndex];

    // Send a proper HTTP response so Chrome can read it
    // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage

    HttpResponse response = RequestHandler::processRequest(request, server);
    std::string responseText = response.serialize();

    if (clients.size() < 2)
    {
        std::cout << "===== HTTP RESPONSE =====\n" << std::endl;
        std::cout << responseText << std::endl;
    }

    bytesSent = send(fd, responseText.c_str(), responseText.size(), 0);
    if (bytesSent >= 0)
    {
        it->second.total_bytesSent += bytesSent;
        if (it->second.total_bytesSent == responseText.size())
            removeClient(fd);
    }
    else
        removeClient(fd);  
}

void Server::removeClient(int fd)
{
    epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    std::map<int, Client>::iterator it = clients.find(fd);
    it->second.buffer.clear();
    clients.erase(fd);
    if (clients.size() < 2)
        std::cout << "Removed client with fd: " << fd << " | clients.size() = " << clients.size() << std::endl;
}

void Server::server_cleanup(void)
{
    // Remove clients from epoll and close sockets
    for (std::map<int, Client>::iterator it = this->clients.begin(); it != this->clients.end(); ++it)
    {
        int fd = it->first;

        epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
    }

    clients.clear();
    // Remove listenning sockets
    for (std::vector<int>::iterator it = this->listeningSockets.begin(); it != this->listeningSockets.end(); it++)
    {
        epoll_ctl(epfd, EPOLL_CTL_DEL, *it, NULL);
        close(*it);
    }

    // Close epoll instance
    close(this->epfd);
}


//YOUTUBE VIDEO
//https://www.youtube.com/watch?v=w2kKgJY4vqY
//EPOLL vs POLL


//https://www.youtube.com/watch?v=wB9tIg209-8&t=303s
//Non-blocking I/O and how Node uses it, in friendly terms: blocking vs async IO, CPU vs IO


//MANUAL 
//https://man7.org/linux/man-pages/man7/epoll.7.html
//epoll(7) — Linux manual page
