#include "configParser.hpp"
#include "server.hpp"
#include <iostream>

int main(int argc, char **argv) {
    const char* configPath = (argc > 1) ? argv[1] : "config/default.conf";

    std::cout << "Parsing: " << configPath << "\n\n";
    try {
        ConfigParser parser(configPath);
        const std::vector<ServerConfig>& servers = parser.getServers();
        Server myServer(servers);
        myServer.start();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

