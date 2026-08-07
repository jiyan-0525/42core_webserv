#include "requestHandler.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <poll.h>
#include <signal.h>

static std::string stripQuery(const std::string& path)
{
    size_t qPos = path.find('?');
    if (qPos == std::string::npos)
        return path;
    return path.substr(0, qPos);
}

static std::string getQuery(const std::string& path)
{
    size_t qPos = path.find('?');
    if (qPos == std::string::npos)
        return "";
    return path.substr(qPos + 1);
}

static std::string joinPath(const std::string& base, const std::string& relative)
{
    if (base.empty())
        return relative;
    if (relative.empty())
        return base;

    bool baseEndsSlash = (base[base.size() - 1] == '/');
    bool relStartsSlash = (relative[0] == '/');

    if (baseEndsSlash && relStartsSlash)
        return base + relative.substr(1);
    if (!baseEndsSlash && !relStartsSlash)
        return base + "/" + relative;
    return base + relative;
}

static std::string parentDirectory(const std::string& path)
{
    size_t slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos)
        return ".";
    if (slashPos == 0)
        return "/";
    return path.substr(0, slashPos);
}

static std::string baseName(const std::string& path)
{
    size_t slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos)
        return path;
    return path.substr(slashPos + 1);
}

static void applyCgiHeadersAndBody(const std::string& cgiOutput, HttpResponse& response)
{
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    size_t sepLen = 4;
    if (headerEnd == std::string::npos)
    {
        headerEnd = cgiOutput.find("\n\n");
        sepLen = 2;
    }

    if (headerEnd == std::string::npos)
    {
        response.setHeader("Content-Type", "text/plain");
        response.setBody(cgiOutput);
        return;
    }

    std::string headerBlock = cgiOutput.substr(0, headerEnd);
    std::string body = cgiOutput.substr(headerEnd + sepLen);

    std::istringstream headersStream(headerBlock);
    std::string line;
    int statusCode = 200;
    std::string statusReason = "OK";
    bool hasContentType = false;

    while (std::getline(headersStream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value.erase(0, 1);

        if (key == "Status")
        {
            std::istringstream statusStream(value);
            statusStream >> statusCode;
            std::getline(statusStream, statusReason);
            while (!statusReason.empty() && statusReason[0] == ' ')
                statusReason.erase(0, 1);
            if (statusReason.empty())
                statusReason = "OK";
        }
        else
        {
            response.setHeader(key, value);
            if (key == "Content-Type")
                hasContentType = true;
        }
    }

    response.setStatus(statusCode, statusReason);
    if (!hasContentType)
        response.setHeader("Content-Type", "text/plain");
    response.setBody(body);
}

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
                        : (code == 500) ? "Internal Server Error"
                        : (code == 504) ? "Gateway Timeout"
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
    std::string cleanPath = stripQuery(req.getPath());
    std::string relativePart = cleanPath.substr(loc.path.size());
    std::string fullPath = joinPath(loc.root, relativePart);

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

HttpResponse RequestHandler::handleCgi(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    std::string cleanPath = stripQuery(req.getPath());
    std::string relativePart = cleanPath.substr(loc.path.size());
    std::string scriptPath = joinPath(loc.root, relativePart);

    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
        return buildError(404, server);

    int inPipe[2];
    int outPipe[2];
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0)
        return buildError(500, server);

    pid_t pid = fork();
    if (pid < 0)
    {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        return buildError(500, server);
    }

    if (pid == 0)
    {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[1]);
        close(outPipe[0]);

        std::string methodEnv = "REQUEST_METHOD=" + req.getMethod();
        std::string queryEnv = "QUERY_STRING=" + getQuery(req.getPath());
        std::string scriptNameEnv = "SCRIPT_NAME=" + cleanPath;
        std::string contentLenEnv = "CONTENT_LENGTH=" + std::to_string(req.getBody().size());
        std::string contentTypeEnv = "CONTENT_TYPE=" + req.getHeader("Content-Type");

        std::vector<char*> envp;
        envp.push_back(const_cast<char*>(methodEnv.c_str()));
        envp.push_back(const_cast<char*>(queryEnv.c_str()));
        envp.push_back(const_cast<char*>(scriptNameEnv.c_str()));
        envp.push_back(const_cast<char*>(contentLenEnv.c_str()));
        envp.push_back(const_cast<char*>(contentTypeEnv.c_str()));
        envp.push_back(nullptr);

        std::string scriptDir = parentDirectory(scriptPath);
        std::string scriptFile = baseName(scriptPath);
        if (chdir(scriptDir.c_str()) != 0)
            _exit(1);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(loc.cgi_path.c_str()));
        argv.push_back(const_cast<char*>(scriptFile.c_str()));
        argv.push_back(nullptr);

        execve(loc.cgi_path.c_str(), argv.data(), envp.data());
        _exit(1);
    }

    close(inPipe[0]);
    close(outPipe[1]);

    if (req.getMethod() == "POST" && !req.getBody().empty())
        write(inPipe[1], req.getBody().c_str(), req.getBody().size());
    close(inPipe[1]);

    std::string cgiOutput;
    char buffer[4096];
    const int timeoutMs = 3000;
    int elapsedMs = 0;
    const int stepMs = 100;

    while (true)
    {
        struct pollfd pfd;
        pfd.fd = outPipe[0];
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pollRes = poll(&pfd, 1, stepMs);
        if (pollRes > 0 && (pfd.revents & POLLIN))
        {
            ssize_t n = read(outPipe[0], buffer, sizeof(buffer));
            if (n > 0)
                cgiOutput.append(buffer, n);
            else if (n == 0)
                break;
        }
        else if (pollRes < 0)
        {
            close(outPipe[0]);
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return buildError(500, server);
        }

        int status = 0;
        pid_t waitRes = waitpid(pid, &status, WNOHANG);
        if (waitRes == pid)
        {
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
                close(outPipe[0]);
                return buildError(500, server);
            }

            while (true)
            {
                ssize_t n = read(outPipe[0], buffer, sizeof(buffer));
                if (n <= 0)
                    break;
                cgiOutput.append(buffer, n);
            }
            break;
        }

        elapsedMs += stepMs;
        if (elapsedMs >= timeoutMs)
        {
            close(outPipe[0]);
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return buildError(504, server);
        }
    }
    close(outPipe[0]);

    HttpResponse response;
    applyCgiHeadersAndBody(cgiOutput, response);
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

    std::string cleanPath = stripQuery(req.getPath());
    if (!loc->cgi_extension.empty() && !loc->cgi_path.empty())
    {
        if (cleanPath.size() >= loc->cgi_extension.size() &&
            cleanPath.substr(cleanPath.size() - loc->cgi_extension.size()) == loc->cgi_extension)
            return handleCgi(req, *loc, server);
    }

    if (method == "GET")
        return handleGet(req, *loc, server);
    if (method == "POST")
        return handlePost(req, *loc, server);
    if (method == "DELETE")
        return handleDelete(req, *loc, server);

    return buildError(501, server);
}