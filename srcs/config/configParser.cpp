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

void ConfigParser::_parse() {
    while (_pos < _tokens.size())
    {
        if (_currenTocken() == "server")
        {
            _consumeToken();
            _servers.push_back(_parseServerBlock());
        }
        else
        {
            throw std::runtime_error("ConfigParser: expected 'server' , got '" + _currenTocken() + "'");
        }
    }
    if (_servers.empty())
    {
        throw std::runtime_error("No server blocks found in configuration.");
    }
}

ServerConfig ConfigParser::_parseServerBlock() {
    ServerConfig server;
    _expectTocken("{");
    while (_pos < _tokens.size() && _currenTocken() != "}")
    {
        std::string directive = _consumeToken();
        if (directive == "listen")
        {
            server.port = std::atoi(_consumeToken().c_str());
            _expectTocken(";");
        }
        else if (directive == "server_name")
        {
            server.server_name = _consumeToken();
            _expectToken(";");
        }
        else if (directive == "client_max_body_size")
        {
            server.client_max_body_size = _parseSize(_consumeToken());
            _expectToken(";");
        }
        else if (directive == "error_page")
        {
            int error_code = std::atoi(_consumeToken().c_str());
            std::string page = _consumeToken();
            server.error_pages[error_code] = page;
            _expectToken(";");
        }
        else if (directive == "location")
        {
            server.locations.push_back(_parseLocationBlock());
        }
        else
        {
            throw std::runtime_error("ConfigParser: unknown directive '" + directive + "'");
        }
    }
    _expectTocken("}");
    return server;
}

LocationConfig ConfigParser:: _parseLocationBlock()
{
    LocationConfig location;
    location.path = _consumeToken();
    _expectToken("{");

    while (_pos < _tokens.size() && _currenTocken() != "}")
    {
        std::string directive = _consumeToken();
        if (directive == "root")
        {
            location.root = _consumeToken();
            _expectToken(";");
        }
        else if (directive == "index")
        {
            location.index = _consumeToken();
            _expectToken(";");
        }
        else if (directive == "methods")
        {
            while (_pos < _tokens.size() && _currenTocken() != ";")
            {
                std::string method = _consumeToken();
                if (method == "GET" || method == "POST" || method == "DELETE")
                {
                    location.methods.push_back(method);
                }
            }
        }
        else if (directive == "autoindex")
        {
            std::string value = _consumeToken();
            if (value == "on")
                location.autoindex = true;
            else if (value == "off")
                location.autoindex = false;
            else
                throw std::runtime_error("ConfigParser: invalid value for autoindex: '" + value + "'");
            _expectToken(";");
        }
        else if (directive == "upload_dir")
        {
            location.upload_dir = _consumeToken();
            _expectToken(";");
        }
        else if (directive == "client_max_body_size")
        {
            location.client_max_body_size = _parseSize(_consumeToken());
            _expectToken(";");
        }
        else if (directive == "cgi_extension")
        {
            location.cgi_extension = _consumeToken();
            _expectToken(";");
        }
        else if (directive == "cgi_path")
        {
            location.cgi_path = _consumeToken();
            _expectToken(";");
        }
        else
        {
            throw std::runtime_error("ConfigParser: unknown directive '" + directive + "'");
        }
    }
    _expectToken("}");
    return location;
}

std::string ConfigParser::_currenTocken() const {
    if (_pos >= _tokens.size())
        throw std::runtime_error("ConfigParser: unexpected end of tokens");
    return _tokens[_pos];
}

std::string ConfigParser::_consumeToken() {
   std::string token = _consumeToken();
    _pos++;
    return token;
}

void ConfigParser::_expectToken(const std::string& expected) {
    std::string token = _consumeToken();
    if (token != expected)
        throw std::runtime_error("ConfigParser: expected '" + expected + "', got '" + token + "'");
}

size_t ConfigParser::_parseSize(const std::string& sizeStr) {
    if (sizeStr.empty())
        return 0;

    char lastChar = sizeStr[sizeStr.size() - 1];

    if (lastChar == 'K' || lastChar == 'k') {
        return std::atoi(sizeStr.c_str()) * 1024;
    } else if (lastChar == 'M' || lastChar == 'm') {
        return std::atoi(sizeStr.c_str()) * 1024 * 1024;
    } else if (lastChar == 'G' || lastChar == 'g') {
        return std::atoi(sizeStr.c_str()) * 1024 * 1024 * 1024;
    } else {
        return std::atoi(sizeStr.c_str());
    }
}