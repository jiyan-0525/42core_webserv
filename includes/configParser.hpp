#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP
# include "Config.hpp"
# include <string>
# include <vector>

class ConfigParser {
public:
    ConfigParser(const std::string& filename);
    ~ConfigParser();
    const std::vector<Config>& getServers() const;

private:
    std::string  _filepath;
    std::vector<std::string> _tokens;
    std::vector<Config> _servers;
    size_t  _pos;
    
    // step 1: read file
   std::string _readFile(const std::string& filepath);
    
   // step 2: tokenize
   void _tokenize(const std::string& content);

   // step 3: parse
   void _parse();
   serverConfig _parseServerBlock();
   locationConfig _parseLocationBlock();

   // helper functions
   std::string _currenTocken() const;
   std::string _consumeToken();
   void _expectToken(const std::string& expected);
   size_t _parseSize(const std::string& sizeStr);

   // non-copyable
   configParser(const configParser& other);
   configParser& operator=(const configParser& other);
    
};


#endif
