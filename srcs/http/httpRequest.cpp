#include "../../includes/httpRequest.hpp"

HttpRequest::HttpRequest() : _method(""), _path(""), _version(""), _body("") {}

HttpRequest::~HttpRequest() {}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    size_t delimiterLen = 4;

    if (headerEnd == std::string::npos)
    {
        headerEnd = rawRequest.find("\n\n");
        delimiterLen = 2;
    }
    std::string headerPart;
    if (headerEnd == std::string::npos) {
        headerPart = rawRequest;
        this->_body = "";
    } else {
        headerPart = rawRequest.substr(0, headerEnd);
        this->_body = rawRequest.substr(headerEnd + delimiterLen);
    }

    std::istringstream requestStream(headerPart);
    std::string requestLine;
    
    if (!std::getline(requestStream, requestLine)) {
        throw std::runtime_error("Invalid HTTP request: empty request line");
    }

    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back(); // Remove trailing carriage return
    }

    std::vector<std::string> requestLineParts = split(requestLine, ' ');
    if (requestLineParts.size() >= 3) {
        this->_method = requestLineParts[0];
        this->_path = requestLineParts[1];
        this->_version = requestLineParts[2];
    } else {
        throw std::runtime_error("Invalid HTTP request: malformed request line");
    }

    while (std::getline(requestStream, requestLine)) {
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back(); // Remove trailing carriage return
        }
        if (requestLine.empty()) {
            continue;
        }
        
        size_t colonPos = requestLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = requestLine.substr(0, colonPos);
            std::string value = requestLine.substr(colonPos + 1);

            // Trim whitespace from key and value
            key.erase(key.find_last_not_of(" \t") + 1);
            key.erase(key.find_first_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            this->_headers[key] = value;
        }
    }
}

const std::string& HttpRequest::getMethod() const {
    return this->_method;
}

const std::string& HttpRequest::getPath() const {
    return this->_path;
}

const std::string& HttpRequest::getVersion() const {
    return this->_version;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return this->_headers;
}

const std::string& HttpRequest::getBody() const {
    return this->_body;
}

const std::string& HttpRequest::getHeader(const std::string& key) const {
    auto it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return it->second;
    }
    static const std::string emptyString = "";
    return emptyString;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return this->_headers.find(key) != this->_headers.end();
}

std::vector<std::string> HttpRequest::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
