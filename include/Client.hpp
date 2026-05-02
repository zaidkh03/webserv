#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "webserv.hpp"
#include "Request.hpp"
#include "Server.hpp"

class Client {
private:
    int _socket;
    Request _request;
    std::string _buffer;
    std::string _response;
    size_t _responseBytesSent;
    time_t _lastActivity;
    const Server* _server;
    bool _keepAlive;

public:
    Client(int socket, const Server* server);
    ~Client();

    int getSocket() const { return _socket; }
    Request& getRequest() { return _request; }
    const Server* getServer() const { return _server; }
    time_t getLastActivity() const { return _lastActivity; }
    bool isKeepAlive() const { return _keepAlive; }
    
    void updateActivity() { _lastActivity = time(NULL); }
    void setResponse(const std::string& response) { 
        _response = response;
        _responseBytesSent = 0;
    }
    
    bool readRequest();
    bool sendResponse();
    bool isTimedOut() const;
    void reset();
};

#endif
