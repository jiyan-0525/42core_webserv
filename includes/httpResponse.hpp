#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include <string>
# include <map>

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, const std::string& reason);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    int getStatusCode() const;
    const std::string& getBody() const;

    // Constructs the entire HTTP body: the data that will be sent directly to `send()`
    std::string serialize() const;

private:
    int statusCode_;
    std::string reasonPhrase_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};

#endif