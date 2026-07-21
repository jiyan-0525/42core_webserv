#include "../../includes/httpRequest.hpp"

HttpRequest::HttpRequest() : _method(""), _path(""), _version(""), _body("") {}

HttpRequest::~HttpRequest() {}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    std::istringstream requestStream(rawRequest);
    std::string requestLine;
    
    if (!std::getline(requestStream, requestLine)) {
        throw std::runtime_error("Invalid HTTP request: empty request line");
    }

    std::vector<std::string> requestLineParts = split(requestLine, ' ');
    if (requestLineParts.size() >= 3) {
        this->_method = requestLineParts[0];
        this->_path = requestLineParts[1];
        this->_version = requestLineParts[2];
    } else {
        throw std::runtime_error("Invalid HTTP request: malformed request line");
    }

    while (std::getline(requestStream, requestLine) && requestLine != "\r") {
        std::cout << "Received request: Happy Parser" << std::endl;           //This line is just to show that the request is successufully arriving to the parser (Lucie)
        if (requestLine.empty() || requestLine == "\r") {
            break;
        }
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back(); // Remove trailing carriage return
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

    this->_body = "";
    std::ostringstream bodyStream;
    while (std::getline(requestStream, requestLine)) {
        bodyStream << requestLine << "\n";
    }
    this->_body = bodyStream.str();
    if (!_body.empty() && this->_body.back() == '\n') {
        this->_body.pop_back();
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
