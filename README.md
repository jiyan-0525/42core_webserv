# Resources

 - https://beej.us/guide/bgnet/

# Explanation

This documentation explains how to connect my application (a C++ server running on my computer) to the network and allow it to communicate through sockets.

The OS already has one or more network interfaces (Ethernet, Wi-Fi, etc.) connected to the network.

Key Word: ```AI_PASSIVE``` -> Bind to all available interfaces (0.0.0.0)
A computer can have multiple network interfaces simultaneously (e.g., Wi-Fi, Ethernet, and the local loopback/localhost).

```
Your application (your C++ server)
        │
        ▼
Socket API (socket, bind, listen, accept)
        │
        ▼
Operating System (kernel)
        │
        ▼
Network Interface Card (Wi-Fi / Ethernet)
        │
        ▼
Network
        │
        ▼
Internet (if reachable)
```


Your application: Calls functions like socket(), bind(), and accept().

Operating System (kernel): Creates and manages sockets, performs the TCP protocol, receives packets, and decides which application should get them.

A socket is an object managed by the operating system that represents one endpoint of a network connection.

When the application (C++ server) calls: ```int server_fd = socket(AF_INET, SOCK_STREAM, 0);```
it is asking the OS: "Please create a TCP listening socket for me."
Then it returns a file descriptor - for the application to refer to the socket the OS created

The function socket() creates a listening socket object inside the OS but first it is empty (like a template):
```
Local IP:    ?
Local Port:  ?
Protocol:    TCP
Remote IP:   none
Remote Port: none
State: Created
```


First step: fill Local IP and Local Port of the socket, in order to do this we need to create a request form

struct addrinfo -> request_form
```
request_form
-----------------------
ai_family    = AF_INET      ✓ IPv4
ai_socktype  = SOCK_STREAM  ✓ TCP
ai_flags     = AI_PASSIVE   ✓ Any local interface


result
-----------------------
(empty)

```
Server code
```getaddrinfo(nullptr, "8080", &request_form, &result);```
"Give me my own address so others can connect to me."
Using these instructions, create an address description and put it in result.

After the call:
```
request_form
-----------------------
ai_family    = AF_INET      ✓ IPv4
ai_socktype  = SOCK_STREAM  ✓ TCP
ai_flags     = AI_PASSIVE   ✓ Any local interface


result
-----------------------
IP address  = 0.0.0.0 ✅
Port        = 8080    ✅
Family      = IPv4    ✅
Socket type = TCP     ✅
```

Now we can bind the socket to the address from result:
```bind(server_fd, result->ai_addr, result->ai_addrlen);```
```
Current status of the listening socket
Local IP:    0.0.0.0 ✅
Local Port:  8080    ✅
Protocol:    TCP
Remote IP:   none
Remote Port: none
State: Bound         ✅
```
Now the socket can start listening from incoming connections (like Chrome): ```listen(server_fd, 3);```
```
Current status of the listening socket
Local IP:    0.0.0.0
Local Port:  8080
Protocol:    TCP
Remote IP:   none
Remote Port: none
State: Listening     ✅
```
The OS receives a TCP connection request from Chrome (in the form of network packets).
It places this pending connection in a waiting queue associated with the listening socket.

IP: 0.0.0.0  
It doesnt mean that "My computer's IP address is 0.0.0.0"  
Open Chrome and go to ```http://localhost:8080\n"```  
localhost = the name humans type  
127.0.0.1 = the IPv4 address computers use  

For example, your computer might actually have:
```
Interface        IP address
----------------------------
Loopback         127.0.0.1
Wi-Fi            192.168.1.25
Ethernet         10.0.0.5
```
which means -> accept connections on:

127.0.0.1:8080  
192.168.1.25:8080  
10.0.0.5:8080  


Chrome has a socket:
```
Local IP:    127.0.0.1
Local Port:  50000
Remote IP:   127.0.0.1
Remote Port: 8080
State:       Connected
```
Local IP and Remote IP have the same IP adress because Chrome and my server are running on the same computer.


NEXT STEP: Accept Chrome's connection ```int client_socket = accept(server_fd, ...);```
accept() does not fill the missing fields of the listening socket. Instead, it creates a new socket that has those remote fields filled.  

VERY IMPORTANT: We have now 2 sockets the first one is the listening socket, the 2nd one was created at the function call accept()  
-> the OS created a new socket the client_socket
```
Client socket

Local IP:    127.0.0.1
Local Port:  8080

Remote IP:   127.0.0.1
Remote Port: 50000

State:       Connected
```

This is the socket you use with:
send(client_socket, ...)
recv(client_socket, ...)

With this client socket we can now have HTTP request and HTTP response

Step 1:
When I type: ```http://localhost:8080``` -> Chrome sends a request  
Chrome creates a TCP connection and then sends something like:
```
GET / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
```
This is an HTTP request and it means: "Server, please give me the resource located at /."

Step 2: Your server receives the request

Step 3: Your server sends a response -> ```send(client_socket, httpResponse, strlen(httpResponse), 0);```

To end the communication, the C++ server closes both sockets. 

```
close(client_socket);
close(server_fd);
return 0;
```


