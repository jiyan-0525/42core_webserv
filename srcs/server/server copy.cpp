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

    // std::vector<Client> clients;
    // size_t clients_count = 0;
    int client_fd = 0;
    ssize_t bytesRecv = 0;
    int ready = 0;



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

    struct epoll_event ctl_event = {
        .events = EPOLLIN | EPOLLOUT,
        .data { .fd = server_fd} //this will give use back the fd of the client that has the event???? No I dont think so, it gives back the server_fd
    };
    //In this epoll event struct we specify the events we want to listen to

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ctl_event); //epoll control -> add a file descriptor to the epoll
    if (ret == -1)
        exit(EXIT_FAILURE);

    while (1)
    {
        struct epoll_event event;
        int events_number = epoll_wait(epfd, &event, 2, 100); // it return the number of events that it has
        if (events_number == -1)
            exit(EXIT_FAILURE);
        else if (events_number == 0)
            continue;
        
        if (event.data.fd == server_fd) // this means there was an event for the server / So we can accept new clients
        {
            //--------------Is there a new client that want to connect?------------------
                client_fd = accept(server_fd, adr, adr_len);
                if (client_fd < 0)
                {
                    perror("In accepting the new client");
                    exit(EXIT_FAILURE);
                }
                // Client client(client_fd);
                // clients.push_back(client);
                // clients_count++;
        }

        // struct pollfd pollfds[MAX_CLIENTS + 1]; // +1 Listenning socket
        // pollfds[0].fd = server_fd;
        // pollfds[0].events = POLLIN; // which event do you want to listen to? POLLOUT to write
        // pollfds[0].revents = 0; //will become non 0, which event is available on this file descriptor

        // for (size_t i = 0; i < clients_count; i++)
        // {
        //     pollfds[i + 1].fd = clients[i].fd; //the first pollfds[0] is the listenning socket
        //     pollfds[i + 1].events = POLLIN | POLLOUT; // which event do you want to listen to? POLLOUT to write
        //     pollfds[i + 1].revents = 0; //will become non 0, which event is available on this file descriptor
        // }

        // //-----Is there a new client that wants to connect OR Is there an already connected client sending an HTTP request?--------
        // ready = poll(pollfds, clients_count + 1, 100);  // this function checks the activity of my listenning socket and all my already connected sockets
        // if (ready == -1)
        // {
        //     perror("Error with a client"); // poll returns the fd number of the client
        //     exit(EXIT_FAILURE);
        //     //NOT OKAY I NEED 
        //     // errno == EAGAIN
        //     // errno == EWOULDBLOCK
        //     //To only handle the problematic client
        // }
        // else if (ready == 0)
        //     continue; // timeout 100ms is reached, we can just continue
        // // ready > 0, one or more fds have events
        // if (pollfds[0].revents == POLLIN) //--------------Is there a new client that want to connect?------------------
        // {
        //     printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        //     client_fd = accept(server_fd, adr, adr_len);
        //     if (client_fd < 0)
        //     {
        //         perror("In accepting the new client");
        //         exit(EXIT_FAILURE);
        //     }
        //     struct epoll_event ctl_event = {
        //         .events = EPOLLIN | EPOLLOUT,
        //         .data { .fd = client_fd} 
        //     };
        //     //In this epoll event struct we specify the events we want to listen to

        //     int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ctl_event); //epoll control -> add a file descriptor to the epoll
 
        //     Client client(client_fd);
        //     clients.push_back(client);
        //     clients_count++;
        // }
        else //-------------Is there an event from an already connected client?
        {
            int client_fd = event.data.fd;

        }
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
                        buffer[bytesRecv] = '\0';      // Make it a C-string
                        clients[i].buffer = buffer;

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
                                clients[i].request.parseRequest(clients[i].buffer);
                                std::cout << "===== HTTP REQUEST =====\n";
                                std::cout << buffer;
                                std::cout << "========================\n";

                                // If we arrive here, parsing succeeded.
                                // Now Person 3 can build the response.
                                //clients[i].response.generateResponse(clients[i].request);
                            }
                            catch (const std::exception& e)
                            {
                                // Parsing failed.
                                // Build a 400 Bad Request response.
                            }
                        //}
                        // HttpRequest request;
                        // request.parseRequest(buffer);

                        // std::cout << "===== HTTP REQUEST =====\n";
                        // std::cout << buffer;
                        // std::cout << "========================\n";
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
                    send(clients[i].fd, httpResponse, strlen(httpResponse), 0);
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






