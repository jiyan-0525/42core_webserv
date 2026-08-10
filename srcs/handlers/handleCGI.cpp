#include "requestHandler.hpp"
#include "requesHanddlerUtils.hpp"
#include <algorithm>
#include <sys/stat.h>
#include <poll.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

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