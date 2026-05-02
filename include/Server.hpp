#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.hpp"
#include "Route.hpp"

class Server {
private:
    std::string _host;
    int _port;
    std::string _serverName;
    size_t _clientMaxBodySize;
    std::map<int, std::string> _errorPages;
    std::vector<Route> _routes;
    int _listenSocket;

public:
    Server();
    ~Server();

    // Getters
    const std::string& getHost() const { return _host; }
    int getPort() const { return _port; }
    const std::string& getServerName() const { return _serverName; }
    size_t getClientMaxBodySize() const { return _clientMaxBodySize; }
    const std::map<int, std::string>& getErrorPages() const { return _errorPages; }
    const std::vector<Route>& getRoutes() const { return _routes; }
    int getListenSocket() const { return _listenSocket; }

    // Setters
    void setHost(const std::string& host) { _host = host; }
    void setPort(int port) { _port = port; }
    void setServerName(const std::string& name) { _serverName = name; }
    void setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
    void addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
    void addRoute(const Route& route) { _routes.push_back(route); }
    void setListenSocket(int socket) { _listenSocket = socket; }

    // Utility
    const Route* matchRoute(const std::string& path) const;
    std::string getErrorPage(int code) const;
    bool setup();
};

#endif
