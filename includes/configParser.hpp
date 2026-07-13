#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP
# include "Config.hpp"
# include <string>
# include <vector>

class ConfigParser {
public:
    static std::vector<Config> parseConfigFile(const std::string& filename);

private:
    static void parserLine(const std::string& line, Config& config);
    static void validate(const std::vector<Config>& configs);
};


#endif
