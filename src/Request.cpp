#include "../include/Request.hpp"

Request::Request() : _complete(false), _headersComplete(false), 
                     _contentLength(0), _chunked(false) {}

Request::~Request() {}

void Request::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> _method >> _uri >> _version;
    
    // Check if parsing was successful
    if (_method.empty() || _uri.empty() || _version.empty()) {
        _method = "INVALID";
        return;
    }
    
    parseURI();
}

void Request::parseHeader(const std::string& line) {
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        
        // Trim whitespace
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos)
            value = value.substr(start);
            
        _headers[key] = value;
    }
}

void Request::parseURI() {
    size_t queryPos = _uri.find('?');
    if (queryPos != std::string::npos) {
        _path = _uri.substr(0, queryPos);
        _queryString = _uri.substr(queryPos + 1);
    } else {
        _path = _uri;
    }
}

bool Request::parseChunkedBody(const std::string& data) {
    static std::string remaining;
    remaining += data;
    
    while (true) {
        size_t crlf = remaining.find("\r\n");
        if (crlf == std::string::npos)
            break;
            
        std::string chunkSizeStr = remaining.substr(0, crlf);
        size_t chunkSize;
        std::istringstream(chunkSizeStr) >> std::hex >> chunkSize;
        
        if (chunkSize == 0) {
            _complete = true;
            return true;
        }
        
        if (remaining.length() < crlf + 2 + chunkSize + 2)
            break;
            
        _body += remaining.substr(crlf + 2, chunkSize);
        remaining = remaining.substr(crlf + 2 + chunkSize + 2);
    }
    
    return false;
}

bool Request::parse(const std::string& data) {
    _rawRequest += data;
    
    if (!_headersComplete) {
        size_t headerEnd = _rawRequest.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return false;
            
        std::string headers = _rawRequest.substr(0, headerEnd);
        std::istringstream stream(headers);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            if (!line.empty() && line[line.length() - 1] == '\r')
                line = line.substr(0, line.length() - 1);
            parseRequestLine(line);
        }
        
        // Parse headers
        while (std::getline(stream, line)) {
            if (!line.empty() && line[line.length() - 1] == '\r')
                line = line.substr(0, line.length() - 1);
            if (!line.empty())
                parseHeader(line);
        }
        
        _headersComplete = true;
        
        // HTTP/1.1 requires Host header
        if (_version == "HTTP/1.1") {
            std::string host = getHeader("Host");
            if (host.empty()) {
                _method = "BADHOST";  // Mark as invalid - missing Host header
            }
        }
        
        // Check for Content-Length
        std::string contentLengthStr = getHeader("Content-Length");
        if (!contentLengthStr.empty())
            _contentLength = std::atoi(contentLengthStr.c_str());
            
        // Check for chunked encoding
        std::string transferEncoding = getHeader("Transfer-Encoding");
        if (transferEncoding == "chunked")
            _chunked = true;
            
        // Get body if present
        if (headerEnd + 4 < _rawRequest.length()) {
            _body = _rawRequest.substr(headerEnd + 4);
        }
    } else {
        _body += data;
    }
    
    // Check if request is complete
    if (_headersComplete) {
        if (_chunked) {
            _complete = parseChunkedBody("");
        } else if (_contentLength > 0) {
            _complete = (_body.length() >= _contentLength);
        } else {
            // No body expected (GET, DELETE, etc.)
            _complete = true;
        }
    }
    
    return _complete;
}

std::string Request::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}
