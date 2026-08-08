#ifndef REHELPER_HPP
# define REHELPER_HPP
# include <string>
# include "httpResponse.hpp"

std::string stripQuery(const std::string& path);
std::string getQuery(const std::string& path);
std::string joinPath(const std::string& base, const std::string& relative);
std::string parentDirectory(const std::string& path);
std::string baseName(const std::string& path);
void applyCgiHeadersAndBody(const std::string& cgiOutput, HttpResponse& response);

#endif
