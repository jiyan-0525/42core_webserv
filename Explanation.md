## Webserv 42 — Full Logic Explained

### 🎯 What is the project?

You're building an HTTP web server from scratch in C++98, similar to a simplified NGINX. When a browser (like Chrome) connects to your server, it should be able to:

- Serve static files (HTML, CSS, images)
- Handle HTTP methods (GET, POST, DELETE)
- Execute CGI scripts (e.g., Python)
- Handle multiple clients simultaneously using non-blocking I/O (`poll()`)
- Be configured via a config file (NGINX-style syntax)

### 🏗️ The Big Picture — The Request/Response Pipeline

Every single thing in your project fits inside this pipeline:

```text
Browser (Chrome) types http://localhost:8080
        │
        ▼
┌─────────────────────────────────────┐
│  1. CONFIG PARSER                   │  ← Read default.conf at startup
│     Loads server settings:          │    (port, root, allowed methods,
│     locations, CGI paths, etc.)     │    error pages, body size limits)
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│  2. SERVER (Networking Layer)       │  ← socket() → bind() → listen()
│     Creates listening sockets for   │     One socket per "listen" port
│     each server block in config     │     (8080, 8081, etc.)
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│  3. EVENT LOOP (poll())             │  ← The HEART of the server
│     Monitors ALL file descriptors   │    Non-blocking: never stuck
│     (listening sockets + clients)   │    waiting on one client
│     Decides: new connection? or     │
│     data from existing client?      │
└─────────────────────────────────────┘
        │                       │
  (new connection)        (data ready)
        │                       │
        ▼                       ▼
  ┌──────────────┐   ┌──────────────────────┐
  │  4. accept() │   │  5. recv() / read()  │
  │  Creates a   │   │  Read raw bytes from │
  │  new client  │   │  existing client     │
  │  socket      │   │                      │
  └──────────────┘   └──────────────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  6. HTTP REQUEST PARSER      │
                │  Raw text → structured obj  │
                │  "GET /index.html HTTP/1.1" │
                │   → method: GET             │
                │   → path: /index.html       │
                │   → headers: Host, etc.     │
                │   → body: (for POST)        │
                └─────────────────────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  7. REQUEST HANDLER (Router) │
                │  Matches path to a location │
                │  block from the config      │
                │                             │
                │  /          → serve www/    │
                │  /files     → directory list│
                │  /upload    → handle upload │
                │  /cgi-bin   → execute CGI   │
                └─────────────────────────────┘
                    │       │       │       │
                    GET    POST   DELETE   CGI
                    │       │       │       │
                    ▼       ▼       ▼       ▼
                ┌─────────────────────────────┐
                │  8. HTTP RESPONSE BUILDER    │
                │  "HTTP/1.1 200 OK\r\n"      │
                │  "Content-Type: text/html"  │
                │  "\r\n"                     │
                │  "<html>...</html>"         │
                └─────────────────────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  9. send() back to client   │
                └─────────────────────────────┘
                              │
                              ▼
                        Browser renders
```

### 📦 Your Project Components Explained

#### 1. Config Parser (`ConfigParser` + `Config`)

**Purpose:** read `default.conf` at startup and turn it into C++ data structures.

```text
default.conf  →  ConfigParser  →  vector<ServerConfig>
```

Each **ServerConfig** holds:

- `listen` port (8080, 8081…)
- `server_name` (localhost, example.com)
- `client_max_body_size` (10M)
- `error_page` map (404 → /errors/404.html)
- `vector<LocationConfig>` — one per `location {}` block

Each **LocationConfig** holds:

- `path` (/, /upload, /cgi-bin)
- `root` directory (./www)
- `index` file (index.html)
- allowed `methods` (GET, POST, DELETE)
- `autoindex` (on/off)
- CGI settings (extension, interpreter path)
- upload directory

#### 2. Server (Networking Layer)

**Purpose:** create sockets and manage the event loop.

```text
// For each server in config:
socket()      // Create a socket
bind()        // Attach it to IP:port
listen()      // Start accepting connections

// Then the main event loop:
while (true) {
    poll(fds, ...)    // Wait for ANY activity on ANY socket

    for each fd with activity:
        if (fd is a listening socket)
            accept()  → create new Client
        else if (fd has data to read)
            recv()    → read into Client's buffer
        else if (fd is ready to write)
            send()    → send response back
}
```

**Why `poll()`?** Without it, `accept()` and `recv()` are blocking — your server freezes waiting for one client and can't serve anyone else. `poll()` tells you which sockets have activity, so you only read/write when data is actually ready. This is the hardest part of the project.

#### 3. Client (Connection State)

**Purpose:** track each connected browser/client.

Each **Client** object holds:

- The client socket fd
- The receive buffer (accumulates bytes until a full HTTP request is ready)
- The parsed `HTTPRequest` (once complete)
- The `HTTPResponse` to send back
- Connection state (reading request? sending response? done?)

#### 4. HTTP Request Parser

**Purpose:** turn raw text into a structured object.

```text
Raw text:                              Parsed object:
"GET /index.html HTTP/1.1\r\n"         method  = "GET"
"Host: localhost:8080\r\n"             path    = "/index.html"
"Content-Length: 0\r\n"                version = "HTTP/1.1"
"\r\n"                                 headers["Host"]  = "localhost:8080"
                                       body    = ""
```

**Key challenges:**

- Detecting when the full request has arrived (look for `\r\n\r\n`)
- Parsing `Content-Length` for POST bodies
- Handling chunked transfer encoding

#### 5. HTTP Response Builder

**Purpose:** construct the response string sent to the client.

For a `GET /index.html`:

```text
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
\r\n
<html>...contents of www/index.html...</html>
```

For a 404:

```text
HTTP/1.1 404 NOT FOUND\r\n
Content-Type: text/html\r\n
Content-Length: 45\r\n
\r\n
<html><body>404 Not Found</body></html>
```

#### 6. CGI (Common Gateway Interface)

**Purpose:** execute external scripts (Python, PHP, etc.).

Request: `GET /cgi-bin/hello.py`

```text
pipe()               → create a pipe for the child's output
fork()               → create a child process

Child process:
    dup2()           → redirect stdout to the pipe
    execve()         → replace itself with: /usr/bin/python3 hello.py

Parent process:
    read(pipe)       → read the script's output
    Build HTTP response from the output
    send() to client
```

### 🗺️ Team split (from README)

| Person   | Responsibility                          | Components                        |
| -------- | --------------------------------------- | --------------------------------- |
| Person 1 | Config parsing + HTTP request parsing    | ConfigParser, Config, HttpRequest |
| Person 2 | Networking + poll() loop + clients        | Server, Client                    |
| Person 3 | Request handling, routing, response, CGI | HttpResponse, RequestHandler, CGI |

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

# Project planification

## TO DO at the end:  

### Step 7: Multiple clients - DIFFICULT  

Don't handle only one browser.  
Handle many simultaneously.  
This is where we will be using things like poll() (or the multiplexing API).  

### Step 8: CGI - DIFFICULT  

Functions needed for that part: fork(), pipe(), dup2(), execve()  
If Chrome asks for ```GET /hello.py```  
The C++ server cannot execute Python, instead:  

```
Chrome
      │
      ▼
C++ Server
      │
      ▼
fork()
      │
      ▼
Child process
      │
      ▼
execve("python", "hello.py")
      │
      ▼
Python outputs HTML
      │
      ▼
Parent server
      │
      ▼
Chrome
```

## TO DO now:  

### Step 1: Establish the connection (Networking) -> Look at the explanation part refering to this topic.  

Small project goal:  
Create a C++ server that uses sockets to communicate through the operating system's network stack.

The server should:    
create a socket  
assign it an address and port  
listen for incoming connections  
accept a client connection  
exchange a small amount of HTTP data with the client   

Example:  
Chrome connects to the server:  

```
Chrome
   |
   | HTTP request
   ▼
C++ Server
   |
   | HTTP response
   ▼
Chrome
```

At the end of this step, the server can successfully establish a TCP connection and send/receive a simple HTTP message.

### Step 2 to 6: The C++ server receives and parses the request, decides what to do, creates and sends an HTTP response.  

### Step 2: Receive the HTTP request - EASY

"What did the client send me?"  
Chrome send an HTTP request  
The C++ server reads the bytes: recv()  
Now the server has the raw text, but it does not understand it yet.  

### Step 3: Receive the HTTP request - Parsing - DIFFICULT

"What does this request mean?"  
The C++ server transforms raw data into a structured object (parsing), to understand the request of the client.  

### Step 4: Process the request - DIFFICULT

"What should I do with this request?"  
According to the "Request" the server decides the "Action".  

### Step 5: Create the HTTP response - DIFFICULT

"How do I format my answer so Chrome understands it?"  
The C++ server needs to generate a formated answer.

### Step 6: Send the HTTP response - EASY

"How do I send my answer back?"  
Your server sends:
```
send(client_socket, response, ...)
```

# The big picture

```
Browser (Chrome)
        │
        ▼
1. Connect to my server
        │
        ▼
2. Send an HTTP request
        │
        ▼
3. My server reads the request
        │
        ▼
4. My server understands the request
        │
        ▼
5. My server decides what to do
        │
        ▼
6. My server generates an HTTP response
        │
        ▼
7. My server sends the response
        │
        ▼
Browser displays the page
```

Everything in the project belongs somewhere in this pipeline.


# Project Organization

## Person 1 – Configuration + HTTP Request Parsing

### Responsibility
This person is responsible for **understanding the server's input**.

### Configuration
- Read the configuration file
- Tokenize it
- Parse it
- Validate it
- Create `ServerConfig`, `LocationConfig`, etc.

### HTTP Request
- Parse the raw HTTP request
- Parse request headers
- Parse the request body
- Handle chunked encoding (if required)
- Build an `HTTPRequest` object

### Output

```text
config.conf
        │
        ▼
Configuration objects

Raw HTTP request
        │
        ▼
HTTPRequest object
```

---

## Person 2 – Networking + Client Management

### Responsibility
This person is responsible for **transporting data between clients and the server**.

### Networking
- `socket()`
- `bind()`
- `listen()`
- `accept()`
- `poll()`

### Client Management
- Handle multiple clients
- Read data from sockets
- Write data to sockets
- Manage the connection lifecycle
- Maintain client state
- Handle keep-alive connections
- Detect when a complete HTTP request has been received

> **Note:** This person does **not** need to understand HTTP methods such as `GET`, `POST`, or `DELETE`. Their job is simply to receive and send bytes.

### Workflow

```text
Client
   │
socket
   │
poll
   │
read bytes
   │
complete HTTP request
   │
pass raw request to parser
```

---

## Person 3 – Response Generation + CGI

### Responsibility
This person is responsible for **deciding how to answer an HTTP request** and producing the final response.

### Request Handling
- Routing
- `GET`
- `POST`
- `DELETE`
- Serve static files
- Directory listing
- MIME type detection
- Error page generation

### CGI
- Detect CGI requests
- Execute CGI programs
- Read CGI output
- Convert CGI output into an HTTP response

### HTTP Response
- Build the `HTTPResponse` object
- Set status code
- Set response headers
- Set response body

### Input / Output

```text
HTTPRequest
        │
        ▼
Request handling
        │
        ├── Static files
        ├── CGI
        ├── Error pages
        └── HTTP methods
        │
        ▼
HTTPResponse
```

---

# Overall Architecture

```text
                Client
                   │
                   ▼
     Person 2 - Network Layer
                   │
           Raw HTTP Request
                   │
                   ▼
 Person 1 - Request Parser
                   │
             HTTPRequest
                   │
                   ▼
 Person 3 - Request Handler
      ├── Routing
      ├── Static Files
      ├── CGI
      ├── GET / POST / DELETE
      └── Error Pages
                   │
             HTTPResponse
                   │
                   ▼
     Person 2 - Network Layer
                   │
                   ▼
                Client
```
```bash
42core_webserv/
├── Makefile
├── includes/
│   ├── Config.hpp          # Config data structures
│   ├── ConfigParser.hpp    # Parser class
│   ├── Server.hpp          # Server class (later)
│   ├── Client.hpp          # Client connection (later)
│   └── HttpRequest.hpp     # HTTP request parser (later)
├── srcs/
│   ├── main.cpp
│   ├── config/
│   │   ├── Config.cpp
│   │   └── ConfigParser.cpp
│   ├── server/
│   │   └── Server.cpp
│   ├── client/
│   │   └── Client.cpp
│   └── http/
│       ├── HttpRequest.cpp
│       └── HttpResponse.cpp
├── config/
│   └── default.conf        # Default config file
└── www/                    # Static website root
    └── index.html
```
