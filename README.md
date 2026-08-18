*This project has been created as part of the 42 curriculum by jiyawang, mhnatovs, lusimon.*

# Description

**webserv** is an HTTP/1.1 web server written in C++, built from scratch as the 42 school *webserv* project. The goal of the project is to implement a functional, non-blocking web server — similar in spirit to a simplified NGINX — capable of handling multiple concurrent clients, serving static files, executing CGI scripts, and processing file uploads.

The server reads an NGINX-style configuration file at startup, opens one listening socket per `server` block, and then runs a single-threaded event loop based on `epoll` (non-blocking I/O). This design allows the server to handle many simultaneous connections without spawning a thread or process per client.

### Key features

- **Non-blocking I/O** with `epoll` — handles multiple clients concurrently in a single event loop
- **NGINX-style configuration** — `server`, `location`, `listen`, `root`, `index`, `methods`, `error_page`, `autoindex`, `upload_dir`, `client_max_body_size`, `cgi_extension`, `cgi_path`
- **HTTP methods** — `GET`, `POST`, `DELETE` with per-location method restrictions
- **Static file serving** — HTML, CSS, images, and plain text with MIME type detection
- **Directory listing** — autoindex output for locations that enable it
- **CGI support** — executes Python scripts via `fork()`, `pipe()`, `dup2()`, and `execve()`, with a 3-second timeout (504 Gateway Timeout on timeout)
- **File upload / POST handling** — saves uploaded bodies to a configurable upload directory
- **Custom error pages** — 403, 404, 500, 504 configured per server
- **Graceful shutdown** — handles `SIGINT`/`SIGTERM` and cleans up all sockets

# Instructions

## Installation

- Linux / macOS with a C++ compiler
- `make`
- Python 3 (for CGI examples)

## Compilation

```bash
make
```

This produces the `webserv` binary. Run `make clean` to remove object files, `make fclean` to remove everything, and `make re` to rebuild from scratch.

## Execution

```bash
./webserv [config_file]
```

If no configuration file is given, the server loads the default configuration:

```bash
./webserv config/default.conf
```

<!-- ### Built-in test configuration

The provided [`config/default.conf`](config/default.conf) defines four virtual servers:

| Port  | server_name  | Serves                          |
|-------|--------------|---------------------------------|
| 8080  | localhost    | Static site, uploads, CGI, data |
| 8081  | example.com  | Static site (`index1.html`)     |
| 8082  | foo.com      | Static site (`index2.html`)     |
| 8084  | test.local   | `www/staticWebsite/`            | -->

## Test HTTP methods

```bash
# GET (list directory with autoindex)
curl -v http://localhost:8080/uploads/
curl -X GET http://localhost:8080/
# POST (submit raw data)
curl -v -X POST -d "test data" http://localhost:8080/data

# Limit the client body test:
# This command creates a file exactly 4 MB

dd if=/dev/zero of=large_file.txt bs=1M count=4

curl -X POST -H "Content-Type: text/plain" --data-binary @large_file.txt http://localhost:8080/files

# DELETE a file
curl -v -X DELETE http://localhost:8080/uploads/myfile.bin
curl -v -X DELETE http://localhost:8080/data/jiyan.txt
```

### Test CGI (Python)

```bash
# GET with query string
curl -s "http://localhost:8080/cgi-bin/test_get.py?name=Jiyan&query=abc"

# POST to a CGI script
curl -v -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=Alice&message=hello" \
  http://localhost:8080/cgi-bin/test_post.py

# POST raw data
curl -s "http://localhost:8080/cgi-bin/test_post.py" -X POST -d "test data"

# Test
curl -i -X POST -d "user=alex&age=25" "http://localhost:8082/cgi-bin/test.py?city=Heilbronn&role=dev"

# Current time example
curl -s http://localhost:8080/cgi-bin/time.py
```

### Stress testing / memory checks

```bash
# Load test with siege (250 concurrent users, 90 seconds)
siege -b -c250 -t90s http://localhost:8080

# Memory leak check with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./webserv
```

## Project structure

```bash
├── Makefile
├── main.cpp
├── README.md
├── config/
│   └── default.conf        # Default NGINX-style configuration
├── includes/
│   ├── config.hpp          # ServerConfig / LocationConfig data structures
│   ├── configParser.hpp    # Configuration file parser
│   ├── httpRequest.hpp     # HTTP request parser
│   ├── httpResponse.hpp    # HTTP response builder
│   ├── requestHandler.hpp  # Routing / method dispatch
│   ├── handleCGI.hpp       # CGI execution
│   ├── requesHanddlerUtils.hpp # Path/query helpers
│   ├── client.hpp          # Client connection state
│   ├── server.hpp          # Event loop (epoll)
│   └── signals.hpp         # Signal handling
├── srcs/
│   ├── config/             # Config parser implementation
│   ├── http/               # HTTP request/response implementation
│   ├── server/             # epoll event loop, client management
│   └── handlers/           # Request handler + CGI implementation
└── www/                    # Web root (static files, CGI scripts, uploads)
```

# Resources

### Here is a practical curl command reference and learning guide, along with the most essential options used in everyday development:

curl -V (or --version)

Purpose: Displays the installed version of curl along with supported protocols and built-in features (like SSL backends and compression support).

Example: curl -V

curl -s (or --silent)

Purpose: Silent mode. It suppresses progress meters and error messages, which is great for keeping output clean in scripts. Combine it with -S (--show-error) to still print errors if things fail.

Example: curl -s -S [https://api.example.com](https://api.example.com)

curl -x (or --proxy)

Purpose: Routes your request through a proxy server (supports HTTP, HTTPS, and SOCKS proxies).

Example: curl -x [http://127.0.0.1:8080](http://127.0.0.1:8080) [https://www.google.com](https://www.google.com)

curl -I (or --head)

Purpose: Fetches only the HTTP response headers without downloading the body content, making it ideal for checking server headers quickly.

Example: curl -I [https://github.com](https://github.com)

curl -X (or --request)

Purpose: Specifies a custom HTTP method for the request (e.g., GET, POST, PUT, DELETE).

Example: curl -X POST -d "data=test" [https://api.example.com](https://api.example.com)

curl -o / -O

Purpose: Saves the output to a file. Use -o filename to specify a custom name, or -O to keep the remote filename.

Example: curl -O [https://example.com/file.zip](https://example.com/file.zip)

## Documentation & references

- **List of HTTP status codes**: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
- **Beej's Guide to Network Programming** — sockets, `bind()`, `listen()`, `accept()`: https://beej.us/guide/bgnet/
- **From the scratch — HTTP server** — what you need to know to build a simple HTTP server: https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa
- **Webserv: building a non-blocking web server in C++ (a 42 project)**: https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7
- **Berkeley sockets** (Wikipedia): https://en.wikipedia.org/wiki/Berkeley_sockets
- **RFC 7230 — HTTP/1.1 Message Syntax and Routing**: https://datatracker.ietf.org/doc/html/rfc7230
- **HTTP methods handbook (GET, POST, DELETE…)**: https://www.freecodecamp.org/news/learn-http-methods-like-get-post-and-delete-a-handbook-with-code-examples/#heading-get-method
- **NGINX configuration syntax** (the style our config format imitates): https://nginx.org/en/docs/beginners_guide.html
- **`epoll` man page / tutorial**: https://man7.org/linux/man-pages/man7/epoll.7.html
- **Common Gateway Interface (CGI)** specification: https://www.rfc-editor.org/rfc/rfc3875

## How AI was used

AI (code-assistance tools) was used during development for the following tasks:

- **Conceptual explanation & planning** — understanding socket programming, the `epoll` event loop, HTTP request/response parsing, and CGI execution flow (`fork`, `pipe`, `dup2`, `execve`).
- **Debugging assistance / Code review** — identifying issues in the event loop (e.g., socket blocking states, partial request reads, `SIGPIPE` handling) and fix suggestions.
- **Writing test/supportive documentation** — generating a summary of the request/response pipeline in `Explanation.md ` curl commands used for manual testing.

All core implementation decisions — the epoll-based event loop, the config parser design, the routing logic, and the CGI timeout handling — were made and implemented by the team. AI was used as an explanatory and debugging companion, not as the author of the project code.
