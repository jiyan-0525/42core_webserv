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


Server::Server(const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        createListeningSocket(servers[i].port);
    }
	initializeEpoll();
}

void Server::createListeningSocket(int port) //the function itself will do listeningSockets.push_back(fd);
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
    std::cout << "Server is running... Open Chrome and go to http://localhost:" << s_port << std::endl;

	listeningSockets.push_back(server_fd);
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
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = 0;
    client_fd = accept(listening_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0)
        return;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct epoll_event client_event = {};
    client_event.events = EPOLLIN;
    client_event.data.fd = client_fd;

    int add = epoll_ctl(this->epfd, EPOLL_CTL_ADD, client_fd, &client_event); //added the new client to the epoll
    if (add == -1)
        return;
    this->clients.insert(std::make_pair(client_fd, Client(client_fd)));
}

bool Server::isListeningSocket(int fd)
{
    std::vector<int>::const_iterator it;
    for (it = this->listeningSockets.cbegin(); it != this->listeningSockets.cend(); it++)
    {
        if (fd == *it)
            return (true);
    }
    return false;
}

static size_t getContentLength(const std::string& buffer)
{
    size_t pos = buffer.find("Content-Length:");

    if (pos == std::string::npos)
        return 0; // No Content-Length header

    pos += std::string("Content-Length:").length();

    // Skip spaces after :
    while (buffer[pos] == ' ')
        pos++;

    size_t end = buffer.find("\r\n", pos);

    std::string length = buffer.substr(pos, end - pos);

    return std::atoi(length.c_str());
}

bool Server::requestComplete(int fd)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return (false);

    size_t headerEnd = it->second.buffer.find("\r\n\r\n");
    // Haven't received all headers yet
    if (headerEnd == std::string::npos)
        return false;
    if (it->second.buffer.find("GET") != std::string::npos || it->second.buffer.find("DELETE") != std::string::npos) //so a position was found
        return (true);
    if (it->second.buffer.find("POST") != std::string::npos)
    {
        size_t contentLength = getContentLength(it->second.buffer);
        size_t bodySize = it->second.buffer.size() - (headerEnd + 4);

        if (bodySize >= contentLength)
            return (true);
    }
    return (false);
}

void Server::receiveData(int fd)
{
    char buffer[RECV_BUFFER_SIZE];
    ssize_t bytesRecv = 0;
                        
    bytesRecv = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRecv == -1 || bytesRecv == 0) //remove this client
    {
        //remove_client_function()
        return;
    }
    else if (bytesRecv > 0)
    {
        buffer[bytesRecv] = '\0';      // Make it a C-string

        std::cout << "here" << std::endl;
        std::map<int, Client>::iterator it = clients.find(fd);
        if (it != clients.end())
        {
            std::cout << "filled the client struct with the HTTP request" << std::endl;
            it->second.buffer += buffer;
        }
        if (requestComplete(fd) == true)
        {
            it->second.request.parseRequest(it->second.buffer);
            std::cout << "===== HTTP REQUEST =====\n";
            std::cout << it->second.buffer;
            std::cout << "========================\n";

            // If we arrive here, parsing succeeded.
            // Now Person 3 can build the response.
            //clients[i].response.generateResponse(clients[i].request);

            // --- (real request, real config) ---
            HttpRequest request;
            request.parseRequest(buffer);              // A REAL request from the browser
        
            ServerConfig server;                       // TEMPORARILY hardcoded, until a real solution is found
            server.port = 8080;                        // config using ConfigParser (this will be connected 
            LocationConfig root;                       // by Person 1 later)
            root.path = "/";
            root.root = "www";
            root.index = "index.html";
            root.methods.push_back("GET");
            server.locations.push_back(root);

            
            // 6. Send a proper HTTP response so Chrome can read it
            // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage
            // The client can almost always receive a response
            //So it is actually better to check if he can receive a response when the response is ready
        
            HttpResponse response = RequestHandler::processRequest(request, server);
            std::string responseText = response.serialize();
        
            std::cout << responseText << std::endl;
            send(fd, responseText.c_str(), responseText.size(), 0);

            //I need to add a condition check
            //if ("Connection: close\r\n" == true)
            //REMOVE CLIENT FUNCTION!!!!
        }
    }
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