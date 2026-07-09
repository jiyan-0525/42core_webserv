#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo request_form;
    struct addrinfo* result;
    
    std::memset(&request_form, 0, sizeof(request_form)); // Zero out memory old-school style
    request_form.ai_family = AF_INET;             // IPv4
    request_form.ai_socktype = SOCK_STREAM;       // TCP
    request_form.ai_flags = AI_PASSIVE;           // Bind to all available interfaces (0.0.0.0)

    char *ptr = NULL;

    if (getaddrinfo(ptr, "8080", &request_form, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        close(server_fd);
        return 1;
    }

    // 3. Bind the socket to the port
    bind(server_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    // 4. Start listening for incoming connections (like Chrome)
    listen(server_fd, 3);
    std::cout << "Server is running... Open Chrome and go to http://localhost:8080\n";

    // 5. Accept Chrome's connection
	sockaddr* adr = NULL;
	socklen_t* adr_len = NULL;
    int client_socket = accept(server_fd, adr, adr_len);

    // 6. Send a proper HTTP response so Chrome can read it
    // The browser needs the "HTTP/1.1 200 OK" header to know it's a valid webpage
    const char* httpResponse = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "Connection: close\r\n"
        "\r\n"
        "HELLO";

        std::cout << httpResponse << std::endl;

    send(client_socket, httpResponse, strlen(httpResponse), 0);

    // 7. Clean up
    close(client_socket);
    close(server_fd);
    return 0;
}
