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

void webserver(const std::vector<ServerConfig>& servers) {

    Server webserver(servers);

    while (1)
    {
        int events_number = epoll_wait(webserver.epfd, webserver.events, 100, 100);

        if (events_number == -1)
            exit(EXIT_FAILURE);
        else if (events_number == 0)
            continue;
        
        for (int i = 0; i < events_number; i++)
        {
            int fd = webserver.events[i].data.fd;

            if (webserver.isListeningSocket(fd) == true)  //--------------Is there a new client that want to connect?------------------
            {
                    std::cout << "here" << std::endl;
                    webserver.acceptNewClient(fd);
            }
            else  //-------------Is there an event from an already connected client?
            {
                std::cout << "Event from an already connected client" << std::endl;
                webserver.receiveData(fd);
            }
        }
    }
    // 7. Clean up
    webserver.server_cleanup();
    return;
}

//NEXT STEPS

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



