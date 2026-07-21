#include "../../includes/httpResponse.hpp"

HttpResponse::HttpResponse()
    : statusCode_(200), reasonPhrase_("OK")
{
}

void HttpResponse::setStatus(int code, const std::string& reason)
{
    statusCode_ = code;
    reasonPhrase_ = reason;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
    headers_[key] = value;
}

void HttpResponse::setBody(const std::string& body)
{
    body_ = body;
    headers_["Content-Length"] = std::to_string(body_.size()); // ок, у нас C++17
}

int HttpResponse::getStatusCode() const { return statusCode_; }
const std::string& HttpResponse::getBody() const { return body_; }

std::string HttpResponse::serialize() const
{
    std::string out = "HTTP/1.1 " + std::to_string(statusCode_) + " " + reasonPhrase_ + "\r\n";

    for (const auto& [key, value] : headers_)
        out += key + ": " + value + "\r\n";

    out += "\r\n" + body_;
    return out;
}
