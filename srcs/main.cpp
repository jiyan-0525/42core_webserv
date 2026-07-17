#include "configParser.hpp"
#include <iostream>

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
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

