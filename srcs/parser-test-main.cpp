#include "configParser.hpp"
#include <iostream>

static void printLocation(const LocationConfig& loc, int idx) {
    std::cout << "    [location " << idx << "]\n";
    std::cout << "      path:               " << loc.path << "\n";
    std::cout << "      root:               " << loc.root << "\n";
    std::cout << "      index:              " << loc.index << "\n";
    std::cout << "      autoindex:          " << (loc.autoindex ? "on" : "off") << "\n";
    std::cout << "      client_max_body:    " << loc.client_max_body_size << " bytes\n";
    std::cout << "      upload_dir:         " << loc.upload_dir << "\n";
    std::cout << "      cgi_extension:      " << loc.cgi_extension << "\n";
    std::cout << "      cgi_path:           " << loc.cgi_path << "\n";
    std::cout << "      methods:            ";
    for (size_t i = 0; i < loc.methods.size(); ++i)
        std::cout << loc.methods[i] << (i + 1 < loc.methods.size() ? ", " : "");
    std::cout << "\n";
}

int main(int argc, char **argv) {
    const char* configPath = (argc > 1) ? argv[1] : "config/default.conf";

    std::cout << "Parsing: " << configPath << "\n\n";
    try {
        ConfigParser parser(configPath);
        const std::vector<ServerConfig>& servers = parser.getServers();

        std::cout << "Found " << servers.size() << " server(s).\n\n";
        for (size_t i = 0; i < servers.size(); ++i) {
            const ServerConfig& s = servers[i];
            std::cout << "=== Server " << i << " ===\n";
            std::cout << "  port:               " << s.port << "\n";
            std::cout << "  server_name:        " << s.server_name << "\n";
            std::cout << "  client_max_body:    " << s.client_max_body_size << " bytes\n";
            std::cout << "  error_pages:\n";
            for (std::map<int,std::string>::const_iterator it = s.error_pages.begin(); it != s.error_pages.end(); ++it)
                std::cout << "    " << it->first << " -> " << it->second << "\n";
            std::cout << "  locations (" << s.locations.size() << "):\n";
            for (size_t j = 0; j < s.locations.size(); ++j)
                printLocation(s.locations[j], j);
            std::cout << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

