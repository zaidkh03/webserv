#include "../include/Response.hpp"

Response::Response() : _statusCode(200) {
    setDefaultHeaders();
}

Response::~Response() {}

void Response::setStatus(int code) {
    _statusCode = code;
    _statusMessage = getStatusMessage(code);
}

std::string Response::getStatusMessage(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

void Response::setDefaultHeaders() {
    _headers["Server"] = "Webserv/1.0";
    // Don't set Connection header here - let it be set based on request
}

std::string Response::build() {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
    
    // Set Content-Length if body exists
    if (!_body.empty()) {
        std::ostringstream contentLength;
        contentLength << _body.length();
        _headers["Content-Length"] = contentLength.str();
    }
    
    // Headers
    for (std::map<std::string, std::string>::iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    response << "\r\n";
    
    // Body
    if (!_body.empty())
        response << _body;
    
    return response.str();
}

std::string Response::buildErrorPage(int code, const Server& server) {
    setStatusCode(code);
    
    std::string errorPagePath = server.getErrorPage(code);
    if (!errorPagePath.empty() && fileExists(errorPagePath)) {
        _body = readFile(errorPagePath);
        _headers["Content-Type"] = getMimeType(errorPagePath);
    } else {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n"
             << "<html>\n<head>\n"
             << "<title>" << code << " " << _statusMessage << "</title>\n"
             << "<style>body{font-family:Arial;text-align:center;padding:50px;}"
             << "h1{color:#d32f2f;}</style>\n"
             << "</head>\n<body>\n"
             << "<h1>" << code << " " << _statusMessage << "</h1>\n"
             << "<p>The requested resource could not be found or accessed.</p>\n"
             << "<hr><p>Webserv/1.0</p>\n"
             << "</body>\n</html>";
        _body = html.str();
        _headers["Content-Type"] = "text/html";
    }
    
    return build();
}

std::string Response::buildRedirect(const std::string& location) {
    setStatusCode(301);
    _headers["Location"] = location;
    
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n<head>\n"
         << "<title>301 Moved Permanently</title>\n"
         << "</head>\n<body>\n"
         << "<h1>Moved Permanently</h1>\n"
         << "<p>The document has moved <a href=\"" << location << "\">here</a>.</p>\n"
         << "</body>\n</html>";
    _body = html.str();
    _headers["Content-Type"] = "text/html";
    
    return build();
}

std::string Response::buildDirectoryListing(const std::string& path, const std::string& uri) {
    setStatusCode(200);
    _headers["Content-Type"] = "text/html";
    
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n<head>\n"
         << "<title>Index of " << uri << "</title>\n"
         << "<style>"
         << "body{font-family:Arial;padding:20px;}"
         << "table{border-collapse:collapse;width:100%;}"
         << "th,td{text-align:left;padding:12px;border-bottom:1px solid #ddd;}"
         << "th{background-color:#667eea;color:white;}"
         << "tr:hover{background-color:#f5f5f5;}"
         << "a{color:#667eea;text-decoration:none;}"
         << "a:hover{text-decoration:underline;}"
         << "</style>\n"
         << "</head>\n<body>\n"
         << "<h1>Index of " << uri << "</h1>\n"
         << "<table>\n"
         << "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n";
    
    // Parent directory
    if (uri != "/") {
        html << "<tr><td><a href=\"../\">../</a></td><td>-</td><td>-</td></tr>\n";
    }
    
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;
                
            std::string fullPath = path + "/" + name;
            struct stat st;
            if (stat(fullPath.c_str(), &st) == 0) {
                bool isDir = S_ISDIR(st.st_mode);
                std::string displayName = name;
                if (isDir)
                    displayName += "/";
                    
                std::ostringstream size;
                if (isDir) {
                    size << "-";
                } else {
                    if (st.st_size < 1024)
                        size << st.st_size << " B";
                    else if (st.st_size < 1048576)
                        size << (st.st_size / 1024) << " KB";
                    else
                        size << (st.st_size / 1048576) << " MB";
                }
                
                char timeStr[100];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
                
                html << "<tr><td><a href=\"" << displayName << "\">" 
                     << displayName << "</a></td>"
                     << "<td>" << size.str() << "</td>"
                     << "<td>" << timeStr << "</td></tr>\n";
            }
        }
        closedir(dir);
    }
    
    html << "</table>\n</body>\n</html>";
    _body = html.str();
    
    return build();
}

std::string Response::getMimeType(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
        
    std::string ext = path.substr(dot);
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".mp3") return "audio/mpeg";
    
    return "application/octet-stream";
}

std::string Response::readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
        
    std::ostringstream contents;
    contents << file.rdbuf();
    file.close();
    
    return contents.str();
}

bool Response::fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

bool Response::isDirectory(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

std::string Response::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream(str.substr(i + 1, 2)) >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}
