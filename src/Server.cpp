#include "../include/Server.hpp"

Server::Server() : _host("0.0.0.0"), _port(8080), _serverName("webserv"),
                   _clientMaxBodySize(1048576), _listenSocket(-1) {}

Server::~Server() {
    if (_listenSocket >= 0)
        close(_listenSocket);
}

const Route* Server::matchRoute(const std::string& path) const {
    const Route* bestMatch = NULL;
    size_t longestMatch = 0;

    for (size_t i = 0; i < _routes.size(); i++) {
        const std::string& routePath = _routes[i].getPath();
        if (path.find(routePath) == 0 && routePath.length() > longestMatch) {
            bestMatch = &_routes[i];
            longestMatch = routePath.length();
        }
    }
    
    return bestMatch;
}

std::string Server::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = _errorPages.find(code);
    if (it != _errorPages.end())
        return it->second;
    return "";
}

bool Server::setup() {
    _listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocket < 0) {
        std::cerr << RED << "Error: Failed to create socket" << RESET << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << RED << "Error: setsockopt failed" << RESET << std::endl;
        close(_listenSocket);
        return false;
    }

    // Set non-blocking
    if (fcntl(_listenSocket, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << RED << "Error: fcntl failed" << RESET << std::endl;
        close(_listenSocket);
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << RED << "Error: Failed to bind to port " << _port << RESET << std::endl;
        close(_listenSocket);
        return false;
    }

    if (listen(_listenSocket, 128) < 0) {
        std::cerr << RED << "Error: listen failed" << RESET << std::endl;
        close(_listenSocket);
        return false;
    }

    std::cout << GREEN << "Server '" << _serverName << "' listening on " 
              << _host << ":" << _port << RESET << std::endl;
    return true;
}
