#include "../include/webserv.hpp"
#include "../include/Config.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Request.hpp"
#include "../include/Response.hpp"
#include "../include/CGI.hpp"
#include "../include/Route.hpp"
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    (void)signum;
    g_running = 0;
}

class WebServer {
private:
    Config _config;
    std::vector<Server> _servers;
    std::vector<struct pollfd> _pollFds;
    std::map<int, Client*> _clients;
    std::map<int, Server*> _serverSockets;

    bool setupServers();
    void acceptConnection(Server* server);
    void handleClient(int clientSocket);
    void removeClient(int socket);
    Response handleRequest(Client* client);
    Response handleGET(const Request& req, const Route* route, const Server& server);
    Response handlePOST(const Request& req, const Route* route, const Server& server);
    Response handleDELETE(const Request& req, const Route* route, const Server& server);
    std::string getFullPath(const std::string& uri, const Route* route);
    bool resolveCgiScript(const Request& req, const Route* route,
                          std::string& cgiPath, std::string& scriptPath,
                          std::string& scriptName, std::string& pathInfo);

public:
    WebServer(const std::string& configFile);
    ~WebServer();
    
    bool initialize();
    void run();
};

WebServer::WebServer(const std::string& configFile) : _config(configFile) {}

WebServer::~WebServer() {
    for (std::map<int, Client*>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        delete it->second;
    }
}

bool WebServer::initialize() {
    if (!_config.parse())
        return false;
        
    _servers = _config.getServers();
    if (!setupServers())
        return false;
    
    return !_servers.empty();
}

bool WebServer::setupServers() {
    if (_servers.empty())
        return false;

    for (size_t i = 0; i < _servers.size(); i++) {
        if (!_servers[i].setup()) {
            std::cerr << RED << "Error: Failed to setup all configured servers" << RESET << std::endl;
            return false;
        }

        struct pollfd pfd;
        pfd.fd = _servers[i].getListenSocket();
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollFds.push_back(pfd);
        _serverSockets[pfd.fd] = &_servers[i];
    }
    
    std::cout << GREEN << "Server initialized with " << _pollFds.size() 
              << " listening socket(s)" << RESET << std::endl;
    return !_pollFds.empty();
}

void WebServer::acceptConnection(Server* server) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(server->getListenSocket(),
                              (struct sockaddr*)&clientAddr,
                              &clientLen);
    
    if (clientSocket < 0)
        return;
        
    if (_clients.size() >= MAX_CLIENTS) {
        close(clientSocket);
        return;
    }
    
    Client* client = new Client(clientSocket, server);
    _clients[clientSocket] = client;
    
    struct pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
    
    std::cout << CYAN << "New connection from " 
              << inet_ntoa(clientAddr.sin_addr) << RESET << std::endl;
}

std::string WebServer::getFullPath(const std::string& uri, const Route* route) {
    std::string decodedUri = Response::urlDecode(uri);

    if (route && !route->getRoot().empty()) {
        std::string suffix = decodedUri;
        const std::string& routePath = route->getPath();
        if (suffix.find(routePath) == 0)
            suffix = suffix.substr(routePath.length());

        if (suffix.empty())
            suffix = "/";
        else if (suffix[0] != '/')
            suffix = "/" + suffix;

        std::string root = route->getRoot();
        if (!root.empty() && root[root.length() - 1] == '/' && !suffix.empty() && suffix[0] == '/')
            return root.substr(0, root.length() - 1) + suffix;
        return root + suffix;
    }

    return "www" + decodedUri;
}

bool WebServer::resolveCgiScript(const Request& req, const Route* route,
                                 std::string& cgiPath, std::string& scriptPath,
                                 std::string& scriptName, std::string& pathInfo) {
    if (!route)
        return false;

    const std::map<std::string, std::string>& cgiExtensions = route->getCgiExtensions();
    if (cgiExtensions.empty())
        return false;

    const std::string& reqPath = req.getPath();
    for (std::map<std::string, std::string>::const_iterator it = cgiExtensions.begin();
         it != cgiExtensions.end(); ++it) {
        const std::string& ext = it->first;
        size_t pos = reqPath.find(ext);
        while (pos != std::string::npos) {
            size_t extEnd = pos + ext.length();
            if (extEnd == reqPath.length() || reqPath[extEnd] == '/') {
                scriptName = reqPath.substr(0, extEnd);
                pathInfo = reqPath.substr(extEnd);
                scriptPath = getFullPath(scriptName, route);
                if (Response::fileExists(scriptPath)) {
                    cgiPath = it->second;
                    return true;
                }
            }
            pos = reqPath.find(ext, pos + 1);
        }
    }

    return false;
}

Response WebServer::handleGET(const Request& req, const Route* route, const Server& server) {
    Response response;
    
    // Check for redirect
    if (route && !route->getRedirect().empty()) {
        return Response();  // Will be handled in handleRequest
    }

    std::string cgiPath;
    std::string scriptPath;
    std::string scriptName;
    std::string pathInfo;
    if (resolveCgiScript(req, route, cgiPath, scriptPath, scriptName, pathInfo)) {
        CGI cgi(cgiPath, scriptPath, scriptName, pathInfo, req, server);
        std::string cgiOutput = cgi.execute();

        if (!cgiOutput.empty()) {
            size_t headerEnd = cgiOutput.find("\r\n\r\n");
            size_t bodyStart = std::string::npos;
            if (headerEnd != std::string::npos)
                bodyStart = headerEnd + 4;
            else {
                headerEnd = cgiOutput.find("\n\n");
                if (headerEnd != std::string::npos)
                    bodyStart = headerEnd + 2;
            }

            if (bodyStart != std::string::npos)
                response.setBody(cgiOutput.substr(bodyStart));
            else
                response.setBody(cgiOutput);
            response.setStatusCode(200);
            response.setHeader("Content-Type", "text/html");
            return response;
        }

        response.setStatusCode(500);
        return response;
    }

    std::string fullPath = getFullPath(req.getPath(), route);

    // Check if path is a directory
    if (Response::isDirectory(fullPath)) {
        // Try index file
        std::string indexPath = fullPath;
        if (indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
            
        std::string indexFile = route ? route->getIndex() : "index.html";
        indexPath += indexFile;
        
        if (Response::fileExists(indexPath)) {
            std::string content = Response::readFile(indexPath);
            response.setStatusCode(200);
            response.setBody(content);
            response.setHeader("Content-Type", Response::getMimeType(indexPath));
            return response;
        }
        
        // Check autoindex
        if (route && route->getAutoindex()) {
            Response listingResponse;
            std::string rawListing = listingResponse.buildDirectoryListing(fullPath, req.getPath());
            size_t headerEnd = rawListing.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
                response.setBody(rawListing.substr(headerEnd + 4));
            response.setStatusCode(200);
            response.setHeader("Content-Type", "text/html");
            return response;
        }
        
        response.setStatusCode(403);
        return response;
    }
    
    // Serve file
    if (Response::fileExists(fullPath)) {
        std::string content = Response::readFile(fullPath);
        response.setStatusCode(200);
        response.setBody(content);
        response.setHeader("Content-Type", Response::getMimeType(fullPath));
    } else {
        response.setStatusCode(404);
    }
    
    return response;
}

Response WebServer::handlePOST(const Request& req, const Route* route, const Server& server) {
    Response response;
    
    // Check body size limit
    size_t maxBodySize = server.getClientMaxBodySize();
    if (route && route->getMaxBodySize() > 0)
        maxBodySize = route->getMaxBodySize();

    if (req.getBody().length() > maxBodySize) {
        response.setStatusCode(413);
        return response;
    }
    
    std::string cgiPath;
    std::string scriptPath;
    std::string scriptName;
    std::string pathInfo;
    if (resolveCgiScript(req, route, cgiPath, scriptPath, scriptName, pathInfo)) {
        CGI cgi(cgiPath, scriptPath, scriptName, pathInfo, req, server);
        std::string cgiOutput = cgi.execute();

        if (!cgiOutput.empty()) {
            size_t headerEnd = cgiOutput.find("\r\n\r\n");
            size_t bodyStart = std::string::npos;
            if (headerEnd != std::string::npos)
                bodyStart = headerEnd + 4;
            else {
                headerEnd = cgiOutput.find("\n\n");
                if (headerEnd != std::string::npos)
                    bodyStart = headerEnd + 2;
            }

            if (bodyStart != std::string::npos)
                response.setBody(cgiOutput.substr(bodyStart));
            else
                response.setBody(cgiOutput);
            response.setStatusCode(200);
            response.setHeader("Content-Type", "text/html");
            return response;
        }

        response.setStatusCode(500);
        return response;
    }
    
    // Handle file upload
    if (route && (route->getUploadEnabled() || !route->getUploadPath().empty())) {
        std::string filePath = getFullPath(req.getPath(), route);
        std::string routePath = route->getPath();
        bool requestTargetsLocationRoot = (req.getPath() == routePath || req.getPath() == routePath + "/");

        if (requestTargetsLocationRoot || Response::isDirectory(filePath)) {
            std::string uploadPath = route->getUploadPath();
            if (uploadPath.empty())
                uploadPath = route->getRoot();

            std::string filename;
            std::string contentType = req.getHeader("Content-Type");
            if (contentType.find("multipart/form-data") != std::string::npos) {
                std::string body = req.getBody();
                size_t filenamePos = body.find("filename=\"");
                if (filenamePos != std::string::npos) {
                    filenamePos += 10;
                    size_t filenameEnd = body.find("\"", filenamePos);
                    if (filenameEnd != std::string::npos) {
                        filename = body.substr(filenamePos, filenameEnd - filenamePos);
                        size_t lastSlash = filename.find_last_of("/\\");
                        if (lastSlash != std::string::npos)
                            filename = filename.substr(lastSlash + 1);
                    }
                }
            }

            if (filename.empty()) {
                std::ostringstream oss;
                oss << time(NULL);
                filename = "upload_" + oss.str();
            }

            if (!uploadPath.empty() && uploadPath[uploadPath.length() - 1] == '/')
                filePath = uploadPath + filename;
            else
                filePath = uploadPath + "/" + filename;
        }

        size_t slashPos = filePath.find_last_of('/');
        if (slashPos != std::string::npos) {
            std::string directory = filePath.substr(0, slashPos);
            mkdir(directory.c_str(), 0755);
        }

        std::ofstream file(filePath.c_str(), std::ios::binary);
        if (file.is_open()) {
            file << req.getBody();
            file.close();
            response.setStatusCode(201);
            response.setBody("{\"status\":\"success\"}");
            response.setHeader("Content-Type", "application/json");
        } else {
            response.setStatusCode(500);
        }
    } else {
        response.setStatusCode(200);
        response.setBody("{\"status\":\"success\"}");
        response.setHeader("Content-Type", "application/json");
    }
    
    return response;
}

Response WebServer::handleDELETE(const Request& req, const Route* route, const Server& server) {
    (void)server;
    Response response;
    
    std::string fullPath = getFullPath(req.getPath(), route);
    
    if (Response::fileExists(fullPath)) {
        if (unlink(fullPath.c_str()) == 0) {
            response.setStatusCode(204);
        } else {
            response.setStatusCode(500);
        }
    } else {
        response.setStatusCode(404);
    }
    
    return response;
}

Response WebServer::handleRequest(Client* client) {
    Response response;
    const Request& req = client->getRequest();
    const Server* server = client->getServer();
    std::string method = req.getMethod();
    bool isHead = (method == "HEAD");
    
    std::cout << YELLOW << req.getMethod() << " " << req.getURI() 
              << " " << req.getVersion() << RESET << std::endl;
    
    // Validate request - check for malformed or invalid method
    if (req.getMethod() == "INVALID") {
        response.setStatusCode(400);  // Bad Request
        response.setHeader("Connection", "close");
        return response;
    }
    
    // Check for missing Host header in HTTP/1.1
    if (req.getMethod() == "BADHOST") {
        response.setStatusCode(400);  // Bad Request - missing Host
        response.setHeader("Connection", "close");
        return response;
    }
    
    // Check for POST without Content-Length
    if (req.getMethod() == "POST" && 
        req.getHeader("Content-Length").empty() && 
        req.getHeader("Transfer-Encoding").empty()) {
        response.setStatusCode(411);  // Length Required
        response.setHeader("Connection", "close");
        return response;
    }
    
    // Set Connection header based on request
    std::string connectionHeader = req.getHeader("Connection");
    if (connectionHeader == "keep-alive") {
        response.setHeader("Connection", "keep-alive");
    } else {
        response.setHeader("Connection", "close");
    }
    
    // Find matching route
    const Route* route = server->matchRoute(req.getPath());
    
    // Check if method is allowed
    if (route) {
        bool allowed = false;
        if (isHead)
            allowed = route->isMethodAllowed("GET");
        else
            allowed = route->isMethodAllowed(method);

        if (!allowed) {
            response.setStatusCode(405);
            response.setBody("");
            return response;
        }
    }

    if (isHead)
        method = "GET";

    // Check for redirect
    if (route && !route->getRedirect().empty()) {
        return Response();  // Special handling in handleClient
    }

    // Handle methods
    if (method == "GET") {
        response = handleGET(req, route, *server);
    }
    else if (method == "POST") {
        response = handlePOST(req, route, *server);
    }
    else if (method == "DELETE") {
        response = handleDELETE(req, route, *server);
    }
    else {
        response.setStatusCode(501);
    }

    // Re-set connection header after method handlers
    if (connectionHeader == "keep-alive") {
        response.setHeader("Connection", "keep-alive");
    } else {
        response.setHeader("Connection", "close");
    }

    return response;
}

static std::string stripResponseBody(const std::string& rawResponse) {
    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return rawResponse;
    return rawResponse.substr(0, headerEnd + 4);
}

void WebServer::handleClient(int clientSocket) {
    Client* client = _clients[clientSocket];
    
    if (client->readRequest()) {
        const Route* route = client->getServer()->matchRoute(client->getRequest().getPath());
        bool isHead = (client->getRequest().getMethod() == "HEAD");
        
        // Handle redirect first
        if (route && !route->getRedirect().empty()) {
            Response redirectResp;
            std::string redirectResponse = redirectResp.buildRedirect(route->getRedirect());
            if (isHead)
                redirectResponse = stripResponseBody(redirectResponse);
            client->setResponse(redirectResponse);
        }
        else {
            Response response = handleRequest(client);
            std::string responseStr = response.build();
            
            // If response indicates an error status, build proper error page
            if (responseStr.find("HTTP/1.1 4") != std::string::npos || 
                responseStr.find("HTTP/1.1 5") != std::string::npos) {
                // Extract status code
                size_t codePos = responseStr.find("HTTP/1.1 ") + 9;
                int statusCode = std::atoi(responseStr.substr(codePos, 3).c_str());
                
                // Build error page with proper status
                std::string errorPage = response.buildErrorPage(statusCode, *client->getServer());
                if (isHead)
                    errorPage = stripResponseBody(errorPage);
                client->setResponse(errorPage);
            }
            else {
                if (isHead)
                    responseStr = stripResponseBody(responseStr);
                // Normal successful response
                client->setResponse(responseStr);
            }
        }
        
        // Switch to write mode
        for (size_t i = 0; i < _pollFds.size(); i++) {
            if (_pollFds[i].fd == clientSocket) {
                _pollFds[i].events = POLLOUT;
                break;
            }
        }
    }
}

void WebServer::removeClient(int socket) {
    std::map<int, Client*>::iterator it = _clients.find(socket);
    if (it != _clients.end()) {
        delete it->second;
        _clients.erase(it);
    }
    
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
         it != _pollFds.end(); ++it) {
        if (it->fd == socket) {
            _pollFds.erase(it);
            break;
        }
    }
}

void WebServer::run() {
    std::cout << GREEN << "Server running... Press Ctrl+C to stop" << RESET << std::endl;
    
    while (g_running) {
        int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
        
        if (ret < 0) {
            if (!g_running)
                break;
            continue;
        }
        
        // Check for timeout clients
        std::vector<int> toRemove;
        for (std::map<int, Client*>::iterator it = _clients.begin();
             it != _clients.end(); ++it) {
            if (it->second->isTimedOut()) {
                std::cout << MAGENTA << "Client timeout" << RESET << std::endl;
                toRemove.push_back(it->first);
            }
        }
        for (size_t i = 0; i < toRemove.size(); i++) {
            removeClient(toRemove[i]);
        }
        
        for (size_t i = 0; i < _pollFds.size(); i++) {
            if (_pollFds[i].revents & POLLIN) {
                // Check if it's a listening socket
                if (_serverSockets.find(_pollFds[i].fd) != _serverSockets.end()) {
                    acceptConnection(_serverSockets[_pollFds[i].fd]);
                } else {
                    handleClient(_pollFds[i].fd);
                }
            }
            else if (_pollFds[i].revents & POLLOUT) {
                int socket = _pollFds[i].fd;
                Client* client = _clients[socket];
                
                if (client && !client->sendResponse()) {
                    removeClient(socket);
                } else {
                    // Switch back to read mode if keep-alive
                    if (client && client->isKeepAlive()) {
                        _pollFds[i].events = POLLIN;
                    }
                }
            }
        }
    }
    
    std::cout << YELLOW << "\nShutting down server..." << RESET << std::endl;
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);
    
    std::string configFile = (argc > 1) ? argv[1] : "config/default.conf";
    
    std::cout << BLUE << "=== Webserv HTTP Server ===" << RESET << std::endl;
    std::cout << "Config file: " << configFile << std::endl;
    
    WebServer server(configFile);
    
    if (!server.initialize()) {
        std::cerr << RED << "Failed to initialize server" << RESET << std::endl;
        return 1;
    }
    
    server.run();
    
    return 0;
}
