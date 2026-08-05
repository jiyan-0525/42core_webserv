#include "requestHandler.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>

// ---------- Helper function to check if a URL matches a location path ----------
static bool isPrefixMatch(const std::string& url, const std::string& locPath)
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
                        : "Error";
    response.setStatus(code, reason);
    response.setHeader("Content-Type", "text/html");

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

// ---------- GET: Return a static file ----------
HttpResponse RequestHandler::handleGet(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    std::string relativePart = req.getPath().substr(loc.path.size());
    std::string fullPath = loc.root + relativePart;

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
        return buildError(404, server);

    if (S_ISDIR(pathStat.st_mode) && !loc.index.empty())
        fullPath += "/" + loc.index;

    std::ifstream file(fullPath);
    if (!file.is_open())
        return buildError(404, server);

    std::ostringstream contents;
    contents << file.rdbuf();

    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", guessMimeType(fullPath));
    response.setBody(contents.str());
    return response;
}

// ---------- POST: save uploaded file ----------
HttpResponse RequestHandler::handlePost(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    if (loc.upload_dir.empty())
        return buildError(403, server);

    std::string filename = "upload_" + std::to_string(rand()) + ".bin";
    if (req.hasHeader("X-Filename")) // optional custom header approach
        filename = req.getHeader("X-Filename");

    std::string fullPath = loc.upload_dir + "/" + filename;

    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open())
        return buildError(500, server);

    outFile.write(req.getBody().c_str(), req.getBody().size());
    outFile.close();

    HttpResponse response;
    response.setStatus(201, "Created");
    response.setHeader("Content-Type", "text/html");
    response.setBody("<html><body><h1>201 Created</h1><p>File uploaded: " + filename + "</p></body></html>");
    return response;
}

// ---------- DELETE: remove a file ----------
HttpResponse RequestHandler::handleDelete(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    std::string relativePart = req.getPath().substr(loc.path.size());
    std::string fullPath = loc.root + relativePart;

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

// ---------- MAIN ENTRY POINT ----------
HttpResponse RequestHandler::processRequest(const HttpRequest& req, const ServerConfig& server)
{
    const LocationConfig* loc = matchLocation(server, req.getPath());
    if (loc == nullptr)
        return buildError(404, server);

    const std::string& method = req.getMethod();
    if (std::find(loc->methods.begin(), loc->methods.end(), method) == loc->methods.end())
        return buildError(405, server);

    if (method == "GET")
        return handleGet(req, *loc, server);
    if (method == "POST")
        return handlePost(req, *loc, server);
    if (method == "DELETE")
        return handleDelete(req, *loc, server);

    return buildError(501, server);
}