#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "config.hpp"
# include "httpRequest.hpp"
# include "httpResponse.hpp"

class RequestHandler {
    public:
        /*This is the main entry point of the RequestHandler. It selects the
        matching location from the server configuration, validates the request,
        dispatches it to the appropriate handler (GET, POST, DELETE, etc.),
        and returns the generated response.*/
        static HttpResponse processRequest(const HttpRequest& req, const ServerConfig& server);
    private:
        static const LocationConfig* matchLocation(const ServerConfig& server, const std::string& url);// Finds the best matching location for a requested URL
        static HttpResponse handleGet(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server);// Handles HTTP GET requests
        static HttpResponse buildError(int code, const ServerConfig& server);// Builds an HTTP error response
        static std::string guessMimeType(const std::string& path);
};

#endif