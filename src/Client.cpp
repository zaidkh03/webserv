#include "../include/Client.hpp"

Client::Client(int socket, const Server* server)
    : _socket(socket), _responseBytesSent(0), _server(server), _keepAlive(false) {
    _lastActivity = time(NULL);
    
    // Set socket to non-blocking
    fcntl(_socket, F_SETFL, O_NONBLOCK);
}

Client::~Client() {
    if (_socket >= 0)
        close(_socket);
}

bool Client::readRequest() {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        return false;
    }
    
    buffer[bytesRead] = '\0';
    updateActivity();
    
    _buffer.append(buffer, bytesRead);
    
    // Try to parse the request
    if (_request.parse(_buffer)) {
        // Check Connection header
        std::string connection = _request.getHeader("Connection");
        if (connection == "keep-alive")
            _keepAlive = true;
        return true;
    }
    
    return false;
}

bool Client::sendResponse() {
    if (_response.empty())
        return true;
        
    ssize_t bytesSent = send(_socket, 
                             _response.c_str() + _responseBytesSent,
                             _response.length() - _responseBytesSent,
                             0);
    
    if (bytesSent < 0) {
        return false;
    }
    
    _responseBytesSent += bytesSent;
    updateActivity();
    
    if (_responseBytesSent >= _response.length()) {
        if (_keepAlive) {
            reset();
            return true;
        }
        return false; // Close connection
    }
    
    return true;
}

bool Client::isTimedOut() const {
    return (time(NULL) - _lastActivity) > REQUEST_TIMEOUT;
}

void Client::reset() {
    _request = Request();
    _buffer.clear();
    _response.clear();
    _responseBytesSent = 0;
    updateActivity();
}
