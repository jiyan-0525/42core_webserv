#include "configParser.hpp"
#include "client.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    const char* configPath;
    if (argc < 2) {
        configPath = "config/default.conf";
    } else {
        configPath = argv[1];
    }

    std::cout << "Parsing: " << configPath << "\n\n";
    try {
        ConfigParser parser(configPath);
        const std::vector<ServerConfig>& servers = parser.getServers();
        std::cout << servers[0].port << std::endl;
        std::cout << servers[1].port << std::endl;

        one_server(servers[0].port);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// #include "../includes/httpRequest.hpp"
// #include <iostream>
// #include <string>

// int main() {
//     // Hardcoded HTTP request (GET request with headers)
//     std::string raw_request =
//         "GET /index.html HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "User-Agent: Mozilla/5.0\r\n"
//         "Accept: text/html\r\n"
//         "Connection: keep-alive\r\n"
//         "\r\n";

//     // Hardcoded HTTP request with a body (POST request)
//     std::string raw_post_request =
//         "POST /submit HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "Content-Type: application/x-www-form-urlencoded\r\n"
//         "Content-Length: 29\r\n"
//         "\r\n"
//         "username=jiyan&password=12345";

//     std::cout << "=== Testing HttpRequest ===\n\n";

//     // Test GET request
//     std::cout << "--- Testing GET Request ---\n";
//     HttpRequest get_request;
//     get_request.parseRequest(raw_request);

//     std::cout << "Method: " << get_request.getMethod() << std::endl;
//     std::cout << "Path: " << get_request.getPath() << std::endl;
//     std::cout << "Version: " << get_request.getVersion() << std::endl;

//     std::cout << "\nHeaders:\n";
//     for (const auto& header : get_request.getHeaders()) {
//         std::cout << "  " << header.first << ": " << header.second << std::endl;
//     }

//     std::cout << "\nBody: " << (get_request.getBody().empty() ? "(empty)" : get_request.getBody()) << std::endl;

//     // Test POST request
//     std::cout << "\n--- Testing POST Request ---\n";
//     HttpRequest post_request;
//     post_request.parseRequest(raw_post_request);

//     std::cout << "Method: " << post_request.getMethod() << std::endl;
//     std::cout << "Path: " << post_request.getPath() << std::endl;
//     std::cout << "Version: " << post_request.getVersion() << std::endl;

//     std::cout << "\nHeaders:\n";
//     for (const auto& header : post_request.getHeaders()) {
//         std::cout << "  " << header.first << ": " << header.second << std::endl;
//     }

//     std::cout << "\nBody: " << (post_request.getBody().empty() ? "(empty)" : post_request.getBody()) << std::endl;

//     return 0;
// }