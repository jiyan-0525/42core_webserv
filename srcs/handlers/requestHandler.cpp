#include "requestHandler.hpp"
#include "requesHanddlerUtils.hpp"
#include "handleCGI.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

// ---------- Helper function to check if a URL matches a location path ----------
bool isPrefixMatch(const std::string& url, const std::string& locPath)
{
    if (url.compare(0, locPath.size(), locPath) != 0)
        return false;
    if (url.size() == locPath.size())
        return true;
    if (locPath.back() == '/')
        return true;
    return url[locPath.size()] == '/';
}

// ---------- Routing: Find the right location based on the URL ----------
const LocationConfig* RequestHandler::matchLocation(const ServerConfig& server,
                                                      const std::string& url)
{
    const LocationConfig*	best = nullptr;
    size_t					bestLen = 0;

    for (const auto& loc : server.locations)
    {
        if (isPrefixMatch(url, loc.path) && loc.path.size() > bestLen)
        {
                best = &loc;
                bestLen = loc.path.size();
        }
    }
    return best;
}

// ---------- Error Pages ----------
HttpResponse RequestHandler::buildError(int code, const ServerConfig& server)
{
    HttpResponse response;
    std::string reason = (code == 404) ? "Not Found"
                        : (code == 403) ? "Forbidden"
                        : (code == 405) ? "Method Not Allowed"
                        : (code == 501) ? "Not Implemented"
                        : (code == 500) ? "Internal Server Error"
                        : (code == 504) ? "Gateway Timeout"
                        : "Error";
    response.setStatus(code, reason);
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Connection", "close");

    auto it = server.error_pages.find(code);
    if (it != server.error_pages.end())
    {
        std::ifstream file(it->second);
        if (file.is_open())
        {
            std::ostringstream contents;
            contents << file.rdbuf();
            response.setBody(contents.str());
            return response;
        }
    }

    response.setBody("<html><body><h1>" + std::to_string(code) + " " + reason + "</h1></body></html>");
    return response;
}

// ---------- MIME type by file extension ----------
std::string RequestHandler::guessMimeType(const std::string& path)
{
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")  return "text/css";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")  return "image/jpeg";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")  return "image/png";
    return "text/plain";
}

HttpResponse RequestHandler::buildDirectoryListing(const std::string& dirPath, const std::string& urlPath, const ServerConfig& server)
{
    (void)server;
    std::ostringstream html;
    html << "<html><head><title>Index of " << urlPath << "</title></head>"
         << "<body><h1>Index of " << urlPath << "</h1><ul>";

    DIR* dir = opendir(dirPath.c_str());
    if (dir != nullptr)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;
            html << "<li><a href=\"" << urlPath;
            if (!urlPath.empty() && urlPath.back() != '/')
                html << "/";
            html << name << "\">" << name << "</a></li>";
        }
        closedir(dir);
    }
    html << "</ul></body></html>";

    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Connection", "close");
    response.setBody(html.str());
    return response;
}

// ---------- GET: Return a static file ----------
HttpResponse RequestHandler::handleGet(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    if (req.getPath().find("..") != std::string::npos)
        return buildError(400, server);

    std::string cleanPath = stripQuery(req.getPath());
    std::string relativePart = cleanPath.substr(loc.path.size());
    std::string fullPath = joinPath(loc.root, relativePart);

    if (loc.root.empty())
        return buildError(500, server);

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
        return buildError(404, server);

    if (S_ISDIR(pathStat.st_mode))
    {
        // checking index-file, if it is specified
        if (!loc.index.empty())
        {
            std::string indexPath = fullPath;
            if (!indexPath.empty() && indexPath.back() != '/')
                indexPath += "/";
            indexPath += loc.index;

            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && !S_ISDIR(indexStat.st_mode))
                fullPath = indexPath;   // index exists - use it
            else if (loc.autoindex)
                return buildDirectoryListing(fullPath, req.getPath(), server); // index not found, but autoindex on
            else
                return buildError(403, server);
        }
        else if (loc.autoindex)
        {
            return buildDirectoryListing(fullPath, cleanPath, server);
        }
        else
        {
            return buildError(403, server);   // no index, no autoindex - nothing to show
        }
    }

    std::ifstream file(fullPath);
    if (!file.is_open())
        return buildError(403, server);

    std::ostringstream contents;
    contents << file.rdbuf();

    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", guessMimeType(fullPath));
    response.setHeader("Connection", "close");
    response.setBody(contents.str());
    return response;
}

// ---------- POST: save uploaded file ----------
HttpResponse RequestHandler::handlePost(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    if (loc.upload_dir.empty())
        return buildError(403, server);

    size_t maxSize = (loc.client_max_body_size > 0) ? loc.client_max_body_size : server.client_max_body_size;
    if (req.getBody().size() > maxSize)
        return buildError(413, server);

    std::string cleanPath = stripQuery(req.getPath());
    std::string filename = cleanPath.substr(loc.path.size());
    
    // Remove leading slash if present
    if (!filename.empty() && filename[0] == '/')
        filename = filename.substr(1);

    // Default to random name if no filename is provided in the URL
    if (filename.empty())
        filename = "upload_" + std::to_string(rand()) + ".bin";

    if (req.hasHeader("X-Filename")) // optional custom header approach
        filename = req.getHeader("X-Filename");

    if (filename.find("..") != std::string::npos)
    return buildError(400, server);

    std::string fullPath = loc.upload_dir + "/" + filename;

    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open())
        return buildError(500, server);

    outFile.write(req.getBody().c_str(), req.getBody().size());
    outFile.close();

    HttpResponse response;
    response.setStatus(201, "Created");
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Connection", "close");
    response.setBody("<html><body><h1>201 Created</h1><p>File uploaded: " + filename + "</p></body></html>");
    return response;
}

// ---------- DELETE: remove a file ----------
HttpResponse RequestHandler::handleDelete(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    std::string cleanPath = stripQuery(req.getPath());
    std::string relativePart = cleanPath.substr(loc.path.size());
    std::string fullPath = joinPath(loc.root, relativePart);

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
        return buildError(404, server);

    if (S_ISDIR(pathStat.st_mode))
        return buildError(403, server);

    if (remove(fullPath.c_str()) != 0)
        return buildError(500, server);

    HttpResponse response;
    response.setStatus(204, "No Content");
    return response;
}

