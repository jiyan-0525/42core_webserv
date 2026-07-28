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
    if (listen(server_fd, SOMAXCONN) < 0) // SOMAXCONN maximum that the Kernel can do 
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is running... Open Chrome and go to http://localhost:8080\n";

    // 5. Accept Chrome's connection
	sockaddr* adr = NULL;
	socklen_t* adr_len = NULL;

    std::map<int, Client> clients;
    int client_fd = 0;
    ssize_t bytesRecv = 0;



//YOUTUBE VIDEO
//https://www.youtube.com/watch?v=w2kKgJY4vqY
//EPOLL vs POLL


//https://www.youtube.com/watch?v=wB9tIg209-8&t=303s
//Non-blocking I/O and how Node uses it, in friendly terms: blocking vs async IO, CPU vs IO


//MANUAL 
//https://man7.org/linux/man-pages/man7/epoll.7.html
//epoll(7) — Linux manual page






    int epfd = epoll_create1(0); //
    if (epfd == -1)
        exit(EXIT_FAILURE);

    struct epoll_event server_event;

        server_event.events = EPOLLIN;
        server_event.data.fd = server_fd;
    //In this epoll server_event struct we specify the events we want to listen to

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &server_event); //epoll control -> add a file descriptor to the epoll
    if (ret == -1)
        exit(EXIT_FAILURE);

    struct epoll_event events[100];

    while (1)
    {

        int events_number = epoll_wait(epfd, events, 100, 100);

        if (events_number == -1)
            exit(EXIT_FAILURE);
        else if (events_number == 0)
            continue;
        
        for (int i = 0; i < events_number; i++)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd)  //--------------Is there a new client that want to connect?------------------
            {
                client_fd = accept(server_fd, adr, adr_len);
                if (client_fd < 0)
                {
                    perror("In accepting the new client");
                    exit(EXIT_FAILURE);
                }

                struct epoll_event client_event;
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                int add = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event); //epoll control -> add a file descriptor to the epoll
                if (add == -1)
                    exit(EXIT_FAILURE);
                clients.insert(std::make_pair(client_fd, Client(client_fd)));
            }
            else  //-------------Is there an event from an already connected client?
            {
                std::cout << "Event from an already connected client" << std::endl;
                client_fd = fd;

                char buffer[RECV_BUFFER_SIZE];
                        
                bytesRecv = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRecv == -1)
                {
                    perror("Reading from the client hereee");
                    exit(EXIT_FAILURE);
                }
                else if (bytesRecv == 0) // the client closed the connection
                {
                    printf("Removing client with fd: %d\n", client_fd);
                    int remove = epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL); // remove from epoll
                    if (remove == -1)
                        exit(EXIT_FAILURE);
                    close(client_fd); // Close socket
                    clients.erase(client_fd); // remove from my map container
                }
                else if (bytesRecv > 0)
                {
                    buffer[bytesRecv] = '\0';      // Make it a C-string

                    std::cout << "here" << std::endl;
                    std::map<int, Client>::iterator it = clients.find(client_fd);
                    if (it != clients.end())
                    {
                        std::cout << "filled the client struct with the HTTP request" << std::endl;
                        it->second.buffer = buffer;
                    }

                    //I need to do my function isRequestComplete()
                    // 2 cases It's either I get a GET request and I wont have any body
                    //either I get everything else (like a post request) and the Content-Length will be specified
                    
                    // if (method == "GET")
                    // {
                    //     if (header_end != std::string::npos)     //if std::string::npos == true == "No position was found."
                    //         request_complete = true;
                    // }

                    // else if (method == "POST")
                    // {
                    //     if (header_end != std::string::npos
                    //         && body_size >= content_length)
                    //         request_complete = true;
                    // }
                    //if (isRequestComplete(clients[i].buffer))
                    //{
                    try
                    {
                        //it->second.request.parseRequest(it->second.buffer);
                        //clients[i].request.parseRequest(clients[i].buffer);
                        std::cout << "===== HTTP REQUEST =====\n";
                        std::cout << it->second.buffer;
                        std::cout << "========================\n";

                        // If we arrive here, parsing succeeded.
                        // Now Person 3 can build the response.
                        //clients[i].response.generateResponse(clients[i].request);
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << "Parsing failed" << std::endl;
                        std::cout << "Build a 400 Bad Request response." << std::endl;
                    }
                //}
                // HttpRequest request;
                // request.parseRequest(buffer);

                // std::cout << "===== HTTP REQUEST =====\n";
                // std::cout << buffer;
                // std::cout << "========================\n";
                }
            }
            // 6. Send a proper HTTP response so Chrome can read it
            // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage
            const char* httpResponse = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 5\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HELLO";
            send(client_fd, httpResponse, strlen(httpResponse), 0);
        }
                // // The client can almost always receive a response
                // //So it is actually better to check if he can receive a response when the response is ready
                // else if (pollfds[i + 1].revents & POLLOUT && answer_ready() == true)
                // {
                //     std::cout << "This client " << clients[i].fd << " can recerive data from the server / This socket is writable" << std::endl;
                //     // 6. Send a proper HTTP response so Chrome can read it
                //     // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage
                //     const char* httpResponse = 
                //     "HTTP/1.1 200 OK\r\n"
                //     "Content-Type: text/plain\r\n"
                //     "Content-Length: 5\r\n"
                //     "Connection: close\r\n"
                //     "\r\n"
                //     "HELLO";
                //     send(clients[i].fd, httpResponse, strlen(httpResponse), 0);
                // }

        //client_list.push_back(client_socket);

    // std::vector<Client>::iterator it;
    }

    // for (it = client_list.begin(); it != client_list.end(); it++)
    // {
    //     std::cout << it->fd << std::endl;
    // }

    // 7. Clean up
    //close(client_socket);
    close(server_fd);
    return;
}


//Non-blocking sockets
//fcntl()
//fcntl(fd, F_SETFL, O_NONBLOCK);

//Error reporting
//errno
//strerror()
//std::cerr << strerror(errno) << std::endl;


//Your program must not crash under any circumstances (even if it 
//runs out of memory) or terminate unexpectedly.

// That means your server should survive situations like:

// a client disconnects unexpectedly
// a client sends malformed data
// a client closes the connection while you're writing
// accept() fails
// recv() fails
// send() fails
// poll() returns an error

// The important word is:
// Handle errors gracefully.


//Never do a read or a write without going through poll().
//You're not checking whether the socket is writable.
//I will need to watch POLLIN and POLLOUT






