#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "webserv.hpp"
#include "Request.hpp"
#include "Server.hpp"

class Response {
private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _response;

    void setStatus(int code);
    std::string getStatusMessage(int code);
    void setDefaultHeaders();

public:
    Response();
    ~Response();

    void setStatusCode(int code) { _statusCode = code; setStatus(code); }
    void setHeader(const std::string& key, const std::string& value) {
        _headers[key] = value;
    }
    void setBody(const std::string& body) { _body = body; }
    
    std::string build();
    std::string buildErrorPage(int code, const Server& server);
    std::string buildRedirect(const std::string& location);
    std::string buildDirectoryListing(const std::string& path, const std::string& uri);
    
    static std::string getMimeType(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool fileExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static std::string urlDecode(const std::string& str);
};

#endif
