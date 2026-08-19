#include "includes/configParser.hpp"
#include "includes/client.hpp"
#include "includes/server.hpp"
#include "includes/signals.hpp"
#include <iostream>
#include <string>
#include <csignal>

int main(int argc, char **argv) {
    srand(time(nullptr));
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGPIPE, SIG_IGN);
    const char* configPath;
    if (argc < 2) {
        configPath = "config/default.conf";
    } else {
        configPath = argv[1];
    }

    try {
        ConfigParser parser(configPath);
        const std::vector<ServerConfig>& servers = parser.getServers();

        Server webserver(servers);
        webserver.eventLoop();

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
