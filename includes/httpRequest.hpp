#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP
# include <iostream>
# include <string>
# include <sstream>
# include <cctype>
# include <vector>
# include <map>

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    void parseRequest(const std::string& rawRequest);

    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    const std::string& getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;

private:
    std::string _method;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;

    static std::vector<std::string> split(const std::string& str, char delimiter);
};


#endif