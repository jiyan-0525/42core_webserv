#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP
# include "config.hpp"
# include <string>
# include <vector>

class ConfigParser {
public:
    ConfigParser(const std::string& filename);
    ~ConfigParser();
    const std::vector<ServerConfig>& getServers() const;

private:
    std::string              _filepath;
    std::vector<std::string> _tokens;
    std::vector<ServerConfig> _servers;
    size_t                   _pos;

    // step 1: read file
    std::string    _readFile(const std::string& filepath);

    // step 2: tokenize
    void           _tokenize(const std::string& content);

    // step 3: parse
    void           _parse();
    ServerConfig   _parseServerBlock();
    LocationConfig _parseLocationBlock();

    // helper functions
    std::string _currentToken() const;
    std::string _consumeToken();
    void        _expectToken(const std::string& expected);
    size_t      _parseSize(const std::string& sizeStr);

    // non-copyable
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);
};

#endif
