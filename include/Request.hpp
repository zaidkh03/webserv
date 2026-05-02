#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "webserv.hpp"

class Request {
private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _rawRequest;
    bool _complete;
    bool _headersComplete;
    size_t _contentLength;
    bool _chunked;
    std::string _queryString;
    std::string _path;

    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    void parseURI();
    bool parseChunkedBody(const std::string& data);

public:
    Request();
    ~Request();

    bool parse(const std::string& data);
    bool isComplete() const { return _complete; }
    
    // Getters
    const std::string& getMethod() const { return _method; }
    const std::string& getURI() const { return _uri; }
    const std::string& getPath() const { return _path; }
    const std::string& getQueryString() const { return _queryString; }
    const std::string& getVersion() const { return _version; }
    const std::map<std::string, std::string>& getHeaders() const { return _headers; }
    const std::string& getBody() const { return _body; }
    std::string getHeader(const std::string& key) const;
    size_t getContentLength() const { return _contentLength; }
    bool isChunked() const { return _chunked; }
};

#endif
