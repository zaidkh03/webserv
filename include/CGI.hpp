#ifndef CGI_HPP
#define CGI_HPP

#include "webserv.hpp"
#include "Request.hpp"
#include "Server.hpp"

class CGI {
private:
    std::string _cgiPath;
    std::string _scriptPath;
    const Request& _request;
    const Server& _server;
    std::map<std::string, std::string> _env;

    void setupEnvironment(const std::string& queryString);
    char** getEnvArray();
    void freeEnvArray(char** env);

public:
    CGI(const std::string& cgiPath, const std::string& scriptPath, 
        const Request& request, const Server& server);
    ~CGI();

    std::string execute();
    std::string executeChunked();
};

#endif
