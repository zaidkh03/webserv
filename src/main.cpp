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

static std::string stripResponseBody(const std::string& rawResponse);

namespace {

std::string trimSpaces(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string toLowerAscii(const std::string& value) {
    std::string lowered = value;
    for (size_t i = 0; i < lowered.length(); i++) {
        if (lowered[i] >= 'A' && lowered[i] <= 'Z')
            lowered[i] = static_cast<char>(lowered[i] - 'A' + 'a');
    }
    return lowered;
}

void applyCgiOutputToResponse(Response& response, const std::string& cgiOutput) {
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    size_t separatorLen = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = cgiOutput.find("\n\n");
        separatorLen = 2;
    }

    int statusCode = 200;
    bool hasContentType = false;
    std::string body;

    if (headerEnd != std::string::npos) {
        std::string headers = cgiOutput.substr(0, headerEnd);
        body = cgiOutput.substr(headerEnd + separatorLen);

        std::istringstream stream(headers);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line[line.length() - 1] == '\r')
                line = line.substr(0, line.length() - 1);
            if (line.empty())
                continue;

            size_t colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = trimSpaces(line.substr(0, colon));
            std::string value = trimSpaces(line.substr(colon + 1));
            std::string lowerKey = toLowerAscii(key);

            if (lowerKey == "status") {
                std::istringstream statusStream(value);
                int parsedStatus = 0;
                statusStream >> parsedStatus;
                if (parsedStatus >= 100 && parsedStatus <= 599)
                    statusCode = parsedStatus;
                continue;
            }

            if (lowerKey == "content-length")
                continue;

            if (lowerKey == "content-type")
                hasContentType = true;

            response.setHeader(key, value);
        }
    } else {
        body = cgiOutput;
    }

    response.setStatusCode(statusCode);
    response.setBody(body);
    if (!hasContentType)
        response.setHeader("Content-Type", "text/html");
}

} // namespace

class WebServer {
private:
    struct CgiTask {
        pid_t workerPid;
        int outputFd;
        int clientSocket;
        bool isHead;
        std::string output;
    };

    Config _config;
    std::vector<Server> _servers;
    std::vector<struct pollfd> _pollFds;
    std::map<int, Client*> _clients;
    std::map<int, Server*> _serverSockets;
    std::map<int, CgiTask> _cgiTasks;
    std::map<int, int> _clientToCgiFd;

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
    bool startAsyncCgi(Client* client, const Route* route, bool isHead);
    void handleCgiOutput(int outputFd);
    void finishCgiTask(int outputFd, bool readError);
    void setPollEvents(int fd, short events);
    void cleanupCgiForClient(int clientSocket);
    bool isCgiOutputFd(int fd) const;

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

bool WebServer::isCgiOutputFd(int fd) const {
    return _cgiTasks.find(fd) != _cgiTasks.end();
}

void WebServer::setPollEvents(int fd, short events) {
    for (size_t i = 0; i < _pollFds.size(); i++) {
        if (_pollFds[i].fd == fd) {
            _pollFds[i].events = events;
            return;
        }
    }
}

void WebServer::cleanupCgiForClient(int clientSocket) {
    std::map<int, int>::iterator mapIt = _clientToCgiFd.find(clientSocket);
    if (mapIt == _clientToCgiFd.end())
        return;

    int outputFd = mapIt->second;
    _clientToCgiFd.erase(mapIt);

    for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
         it != _pollFds.end(); ++it) {
        if (it->fd == outputFd) {
            _pollFds.erase(it);
            break;
        }
    }

    std::map<int, CgiTask>::iterator taskIt = _cgiTasks.find(outputFd);
    if (taskIt != _cgiTasks.end()) {
        pid_t workerPid = taskIt->second.workerPid;
        close(outputFd);
        if (workerPid > 0)
            kill(workerPid, SIGKILL);
        int status = 0;
        if (workerPid > 0)
            waitpid(workerPid, &status, 0);
        _cgiTasks.erase(taskIt);
    }
}

bool WebServer::startAsyncCgi(Client* client, const Route* route, bool isHead) {
    if (!client || !route || !route->getRedirect().empty())
        return false;

    const Request& req = client->getRequest();
    if (req.getMethod() == "INVALID" || req.getMethod() == "BADHOST")
        return false;

    std::string effectiveMethod = req.getMethod();
    if (isHead)
        effectiveMethod = "GET";

    if (effectiveMethod != "GET" && effectiveMethod != "POST")
        return false;

    bool allowed = isHead ? route->isMethodAllowed("GET") : route->isMethodAllowed(effectiveMethod);
    if (!allowed)
        return false;

    if (effectiveMethod == "POST" &&
        req.getHeader("Content-Length").empty() &&
        req.getHeader("Transfer-Encoding").empty()) {
        return false;
    }

    if (effectiveMethod == "POST") {
        size_t maxBodySize = client->getServer()->getClientMaxBodySize();
        if (route->getMaxBodySize() > 0)
            maxBodySize = route->getMaxBodySize();

        if (req.getBody().length() > maxBodySize) {
            Response response;
            if (req.getHeader("Connection") == "keep-alive")
                response.setHeader("Connection", "keep-alive");
            else
                response.setHeader("Connection", "close");
            std::string errorPage = response.buildErrorPage(413, *client->getServer());
            if (isHead)
                errorPage = stripResponseBody(errorPage);
            client->setResponse(errorPage);
            setPollEvents(client->getSocket(), POLLOUT);
            return true;
        }
    }

    std::string cgiPath;
    std::string scriptPath;
    std::string scriptName;
    std::string pathInfo;
    if (!resolveCgiScript(req, route, cgiPath, scriptPath, scriptName, pathInfo))
        return false;

    int outputPipe[2];
    if (pipe(outputPipe) < 0) {
        Response response;
        if (req.getHeader("Connection") == "keep-alive")
            response.setHeader("Connection", "keep-alive");
        else
            response.setHeader("Connection", "close");
        std::string errorPage = response.buildErrorPage(500, *client->getServer());
        if (isHead)
            errorPage = stripResponseBody(errorPage);
        client->setResponse(errorPage);
        setPollEvents(client->getSocket(), POLLOUT);
        return true;
    }

    pid_t workerPid = fork();
    if (workerPid < 0) {
        close(outputPipe[0]);
        close(outputPipe[1]);
        Response response;
        if (req.getHeader("Connection") == "keep-alive")
            response.setHeader("Connection", "keep-alive");
        else
            response.setHeader("Connection", "close");
        std::string errorPage = response.buildErrorPage(500, *client->getServer());
        if (isHead)
            errorPage = stripResponseBody(errorPage);
        client->setResponse(errorPage);
        setPollEvents(client->getSocket(), POLLOUT);
        return true;
    }

    if (workerPid == 0) {
        for (size_t i = 0; i < _pollFds.size(); i++) {
            int fd = _pollFds[i].fd;
            if (fd > 2 && fd != outputPipe[1])
                close(fd);
        }

        close(outputPipe[0]);
        CGI cgi(cgiPath, scriptPath, scriptName, pathInfo, req, *client->getServer());
        std::string cgiOutput = cgi.execute();
        if (!cgiOutput.empty()) {
            const char* data = cgiOutput.c_str();
            size_t total = 0;
            while (total < cgiOutput.length()) {
                ssize_t written = write(outputPipe[1], data + total, cgiOutput.length() - total);
                if (written <= 0)
                    break;
                total += static_cast<size_t>(written);
            }
        }
        close(outputPipe[1]);
        _exit(cgiOutput.empty() ? 1 : 0);
    }

    close(outputPipe[1]);

    struct pollfd pfd;
    pfd.fd = outputPipe[0];
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);

    CgiTask task;
    task.workerPid = workerPid;
    task.outputFd = outputPipe[0];
    task.clientSocket = client->getSocket();
    task.isHead = isHead;
    _cgiTasks[outputPipe[0]] = task;
    _clientToCgiFd[client->getSocket()] = outputPipe[0];

    // Pause socket activity until CGI is done.
    setPollEvents(client->getSocket(), 0);
    return true;
}

void WebServer::finishCgiTask(int outputFd, bool readError) {
    std::map<int, CgiTask>::iterator taskIt = _cgiTasks.find(outputFd);
    if (taskIt == _cgiTasks.end())
        return;

    CgiTask task = taskIt->second;
    _cgiTasks.erase(taskIt);

    std::map<int, int>::iterator clientMapIt = _clientToCgiFd.find(task.clientSocket);
    if (clientMapIt != _clientToCgiFd.end() && clientMapIt->second == outputFd)
        _clientToCgiFd.erase(clientMapIt);

    for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
         it != _pollFds.end(); ++it) {
        if (it->fd == outputFd) {
            _pollFds.erase(it);
            break;
        }
    }

    close(outputFd);

    int workerStatus = 0;
    pid_t waited = waitpid(task.workerPid, &workerStatus, WNOHANG);
    if (waited == 0)
        waited = waitpid(task.workerPid, &workerStatus, 0);

    std::map<int, Client*>::iterator clientIt = _clients.find(task.clientSocket);
    if (clientIt == _clients.end())
        return;

    Client* client = clientIt->second;
    Response response;
    if (client->getRequest().getHeader("Connection") == "keep-alive")
        response.setHeader("Connection", "keep-alive");
    else
        response.setHeader("Connection", "close");

    bool workerOk = (waited > 0 && WIFEXITED(workerStatus) && WEXITSTATUS(workerStatus) == 0);
    bool success = (!readError && workerOk && !task.output.empty());

    std::string rawResponse;
    if (success) {
        applyCgiOutputToResponse(response, task.output);
        rawResponse = response.build();
    } else {
        rawResponse = response.buildErrorPage(500, *client->getServer());
    }

    if (task.isHead)
        rawResponse = stripResponseBody(rawResponse);
    client->setResponse(rawResponse);
    setPollEvents(task.clientSocket, POLLOUT);
}

void WebServer::handleCgiOutput(int outputFd) {
    std::map<int, CgiTask>::iterator taskIt = _cgiTasks.find(outputFd);
    if (taskIt == _cgiTasks.end())
        return;

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = read(outputFd, buffer, sizeof(buffer));
    if (bytesRead > 0) {
        taskIt->second.output.append(buffer, bytesRead);
        return;
    }

    if (bytesRead == 0)
        finishCgiTask(outputFd, false);
    else
        finishCgiTask(outputFd, true);
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
            applyCgiOutputToResponse(response, cgiOutput);
            return response;
        }

        response.setStatusCode(500);
        return response;
    }

    std::string fullPath = getFullPath(req.getPath(), route);

    // Check if path is a directory
    if (Response::isDirectory(fullPath)) {
        // Autoindex has explicit priority when enabled on this route.
        // This makes autoindex deterministic even if an index file exists.
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
            applyCgiOutputToResponse(response, cgiOutput);
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
    std::map<int, Client*>::iterator clientIt = _clients.find(clientSocket);
    if (clientIt == _clients.end())
        return;
    Client* client = clientIt->second;
    
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
        else if (startAsyncCgi(client, route, isHead)) {
            return;
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
        setPollEvents(clientSocket, POLLOUT);
    } else if (client->isDisconnected()) {
        removeClient(clientSocket);
    }
}

void WebServer::removeClient(int socket) {
    cleanupCgiForClient(socket);

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
            int fd = _pollFds[i].fd;

            if (isCgiOutputFd(fd) && (_pollFds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
                handleCgiOutput(fd);
                continue;
            }

            if (_pollFds[i].revents & POLLIN) {
                // Check if it's a listening socket
                if (_serverSockets.find(fd) != _serverSockets.end()) {
                    acceptConnection(_serverSockets[fd]);
                } else {
                    handleClient(fd);
                }
            }
            else if (_pollFds[i].revents & POLLOUT) {
                int socket = fd;
                std::map<int, Client*>::iterator clientIt = _clients.find(socket);
                Client* client = (clientIt != _clients.end()) ? clientIt->second : NULL;
                
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
