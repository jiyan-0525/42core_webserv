#include "ConfigParser.hpp"

static std::vector<Config> ConfigParser::parseConfigFile(const std::string& filename) {
    std::vector<Config> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }
}

