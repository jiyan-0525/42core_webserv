#include "../../includes/server.hpp"
#include "../../includes/client.hpp"
#include "../../includes/httpRequest.hpp"
#include "../../includes/httpResponse.hpp"
#include "../../includes/requestHandler.hpp"
#include "../../includes/config.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_CLIENTS 1024
#define RECV_BUFFER_SIZE 1024

void server_cleanup(std::map<int, Client>& clients, int epfd, int server_fd)
{
    // Remove clients from epoll and close sockets
    for (std::map<int, Client>::iterator it = clients.begin();
         it != clients.end();
         ++it)
    {
        int fd = it->first;

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
    }

    clients.clear();

    // Remove server socket
    epoll_ctl(epfd, EPOLL_CTL_DEL, server_fd, NULL);
    close(server_fd);

    // Close epoll instance
    close(epfd);
}

void webserver(const std::vector<ServerConfig>& servers) {

    Server webserver(servers);

    // 5. Accept Chrome's connection
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::map<int, Client> clients;
    int client_fd = 0;
    ssize_t bytesRecv = 0;

    struct epoll_event events[100];

    while (1)
    {

        int events_number = epoll_wait(webserver.epfd, events, 100, 100);

        if (events_number == -1)
            exit(EXIT_FAILURE);
        else if (events_number == 0)
            continue;
        
        for (int i = 0; i < events_number; i++)
        {
            int fd = events[i].data.fd;

            if (fd == webserver.listeningSockets[0])  //--------------Is there a new client that want to connect?------------------
            {
                client_fd = accept(webserver.listeningSockets[0], (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0)
                {
                    perror("In accepting the new client");
                    exit(EXIT_FAILURE);
                }
                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                struct epoll_event client_event;
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                int add = epoll_ctl(webserver.epfd, EPOLL_CTL_ADD, client_fd, &client_event); //epoll control -> add a file descriptor to the epoll
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
                if (bytesRecv == -1) //remove this client
                {
                    perror("recv");
                    
                    epoll_ctl(webserver.epfd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    clients.erase(client_fd);

                    continue;
                }
                else if (bytesRecv == 0) // the client closed the connection
                {
                    printf("Removing client with fd: %d\n", client_fd);
                    epoll_ctl(webserver.epfd, EPOLL_CTL_DEL, client_fd, NULL); // remove from epoll
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
                    //         && body_size == content_length)
                    //         request_complete = true;
                    // }
                    //if (isRequestComplete(clients[i].buffer))
                    //{
                    try
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
                        send(client_fd, responseText.c_str(), responseText.size(), 0);

                        //I need to add a condition check
                        //if ("Connection: close\r\n" == true)
                        epoll_ctl(webserver.epfd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                        clients.erase(client_fd);

                    }
                    catch (const std::exception& e)
                    {
                        std::cout << "Parsing failed" << std::endl;
                        std::cout << "Build a 400 Bad Request response." << std::endl;
                    }
                }
            }
        }
    }
    // 7. Clean up
    server_cleanup(clients, webserver.epfd, webserver.listeningSockets[0]);
    return;
}

//NEXT STEPS

//Make sockets non-blocking
// #include <fcntl.h>
// fcntl(server_fd, F_SETFL, O_NONBLOCK);

//I close too early
//if ("Connection: close\r\n" == false)
//The connection with my client needs to be kept alive


//YOUTUBE VIDEO
//https://www.youtube.com/watch?v=w2kKgJY4vqY
//EPOLL vs POLL


//https://www.youtube.com/watch?v=wB9tIg209-8&t=303s
//Non-blocking I/O and how Node uses it, in friendly terms: blocking vs async IO, CPU vs IO


//MANUAL 
//https://man7.org/linux/man-pages/man7/epoll.7.html
//epoll(7) — Linux manual page

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

// try
// {
//     /* code */
// }
// catch(const std::exception& e)
// {
//     std::cerr << e.what() << '\n';
// }

// Some-Header: value\r\n
// Some-Header: value\r\n
// Body-Size: 134\r\n
// \r\n
// {
// }
// 



