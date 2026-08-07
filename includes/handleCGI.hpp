#ifndef REHELPER_HPP
# define REHELPER_HPP
# include <string>
# include "httpResponse.hpp"

HttpResponse RequestHandler::handleCgi(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server);
HttpResponse RequestHandler::processRequest(const HttpRequest& req, const ServerConfig& server);

#endif