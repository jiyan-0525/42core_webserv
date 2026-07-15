#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string& filename) : _filepath(filename), _pos(0) {
    std::string content = _readFile(filename);
    _tokenize(content);
    _parse();
}

ConfigParser::~ConfigParser() {}

const std::vector<Config>& ConfigParser::getServers() const {
    return _servers;
}

// step 1: read file
std::string ConfigParser::_readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filepath);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    return ss.str();
}

// step2: tokenize
void ConfigParser::_tokenize(const std::string& content) {
    std::string current;
    for (size_t i = 0; i < content.size(); i++)
    {
        char = content[i];
        if (c == '#')
        {
            while (i < content.size() && content[i] != '\n')
                i++;
            continue;
        }
        if (c =='{' || c == '}' || c == ';')
        {
            if (!current.empty())
            {
                _tokens.push_back(current);
                current.clear();
            }
            _tokens.push_back(std::string(1, c));
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if (!current.empty())
            {
                _tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += c;
    }
    if (!current.empty())
        _tokens.push_back(current);
}

// step 3: parser

