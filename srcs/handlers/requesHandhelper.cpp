#include "requesHandhelper.hpp"
#include <sstream>

std::string stripQuery(const std::string& path)
{
    size_t qPos = path.find('?');
    if (qPos == std::string::npos)
        return path;
    return path.substr(0, qPos);
}

std::string getQuery(const std::string& path)
{
    size_t qPos = path.find('?');
    if (qPos == std::string::npos)
        return "";
    return path.substr(qPos + 1);
}

std::string joinPath(const std::string& base, const std::string& relative)
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

std::string parentDirectory(const std::string& path)
{
    size_t slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos)
        return ".";
    if (slashPos == 0)
        return "/";
    return path.substr(0, slashPos);
}

std::string baseName(const std::string& path)
{
    size_t slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos)
        return path;
    return path.substr(slashPos + 1);
}

void applyCgiHeadersAndBody(const std::string& cgiOutput, HttpResponse& response)
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
