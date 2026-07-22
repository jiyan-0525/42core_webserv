#include "../../includes/server.hpp"
#include "../../includes/client.hpp"
#include "../../includes/httpRequest.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <poll.h>

#include "httpResponse.hpp"
#include "requestHandler.hpp"
#include "config.hpp"

#define MAX_CLIENTS 1024
#define RECV_BUFFER_SIZE 1024

void one_server(int port) {

    std::string s_port = std::to_string(port);

    int server_fd;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

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
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    // 4. Start listening for incoming connections (like Chrome)
    //int listen(int socket, int backlog);
    //The second parameter, backlog, defines the maximum number of pending connections that can be queued up before connections are refused.
    if (listen(server_fd, 3) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is running... Open Chrome and go to http://localhost:8080\n";

    // 5. Accept Chrome's connection
	sockaddr* adr = NULL;
	socklen_t* adr_len = NULL;

    std::vector<Client> clients;
    size_t clients_count = 0;
    int client_fd = 0;
    ssize_t bytesRecv = 0;
    int ready = 0;

    while (1)
    {
        struct pollfd pollfds[MAX_CLIENTS + 1]; // +1 Listenning socket
        pollfds[0].fd = server_fd;
        pollfds[0].events = POLLIN; // which event do you want to listen to? POLLOUT to write
        pollfds[0].revents = 0; //will become non 0, which event is available on this file descriptor

        for (size_t i = 0; i < clients_count; i++)
        {
            pollfds[i + 1].fd = clients[i].fd; //the first pollfds[0] is the listenning socket
            pollfds[i + 1].events = POLLIN; // which event do you want to listen to? POLLOUT to write
            pollfds[i + 1].revents = 0; //will become non 0, which event is available on this file descriptor
        }

        //-----Is there a new client that wants to connect OR Is there an already connected client sending an HTTP request?--------
        ready = poll(pollfds, clients_count + 1, 100);  // this function checks the activity of my listenning socket and all my already connected sockets
        if (ready == -1)
        {
            perror("Error with a client"); // poll returns the fd number of the client
            exit(EXIT_FAILURE);
        }
        else if (ready == 0)
            continue; // timeout 100ms is reached, we can just continue
        else // ready > 0, one or more fds have events
        {
            //--------------Is there a new client that want to connect?------------------
            if (pollfds[0].revents == POLLIN)
            {
                printf("\n+++++++ Waiting for new connection ++++++++\n\n");
                client_fd = accept(server_fd, adr, adr_len);
                if (client_fd < 0)
                {
                    perror("In accepting the new client");
                    exit(EXIT_FAILURE);
                }
                Client client(client_fd);
                clients.push_back(client);
                clients_count++;
            }
            //-------------Is there an event from an already connected client?
            for (size_t i = 0; i < clients_count; i++)
            {
                if (pollfds[i + 1].revents & POLLIN) //revents & POLLIN can accept a less strict condition -> for example revents = POLLIN | POLLHUP at the same time
                {
                    char buffer[RECV_BUFFER_SIZE];
                    
                    bytesRecv = recv(clients[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytesRecv == -1)
                    {
                        perror("Reading from the client hereee");
                        exit(EXIT_FAILURE);
                    }
                    else if (bytesRecv == 0) // the client closed the connection
                    {
                        printf("Removing client with fd: %d\n", clients[i].fd);
                        close(clients[i].fd);
                        clients.erase(clients.begin() + i);
                        //clients_fd[i] = clients_fd[clients_count - 1]; // I need to also remove the client_fd from the list of fd that poll() needs to check
                        clients_count--;
                        i--;
                    }
                    else if (bytesRecv > 0)
                    {
                        buffer[bytesRecv] = '\0';
                    
                        std::cout << "===== HTTP REQUEST =====\n";
                        std::cout << buffer;
                        std::cout << "========================\n";
                    
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
                    
                        HttpResponse response = RequestHandler::processRequest(request, server);
                        std::string responseText = response.serialize();
                    
                        send(clients[i].fd, responseText.c_str(), responseText.size(), 0);
                    }
                    // 6. Send a proper HTTP response so Chrome can read it
                    // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage
                    // const char* httpResponse = 
                    // "HTTP/1.1 200 OK\r\n"
                    // "Content-Type: text/plain\r\n"
                    // "Content-Length: 5\r\n"
                    // "Connection: close\r\n"
                    // "\r\n"
                    // "HELLO";
                    // send(clients[i].fd, httpResponse, strlen(httpResponse), 0);
                }
            }
        //client_list.push_back(client_socket);
        }
    }

    // std::vector<Client>::iterator it;

    // for (it = client_list.begin(); it != client_list.end(); it++)
    // {
    //     std::cout << it->fd << std::endl;
    // }

    // 7. Clean up
    //close(client_socket);
    close(server_fd);
    return;
}
