#ifndef ROUTE_HPP
#define ROUTE_HPP

#include "webserv.hpp"

class Route {
private:
    std::string _path;
    std::vector<std::string> _methods;
    std::string _root;
    std::string _index;
    bool _autoindex;
    std::string _redirect;
    std::string _uploadPath;
    std::map<std::string, std::string> _cgiExtensions;

public:
    Route();
    Route(const std::string& path);
    ~Route();

    // Getters
    const std::string& getPath() const { return _path; }
    const std::vector<std::string>& getMethods() const { return _methods; }
    const std::string& getRoot() const { return _root; }
    const std::string& getIndex() const { return _index; }
    bool getAutoindex() const { return _autoindex; }
    const std::string& getRedirect() const { return _redirect; }
    const std::string& getUploadPath() const { return _uploadPath; }
    const std::map<std::string, std::string>& getCgiExtensions() const { return _cgiExtensions; }

    // Setters
    void setPath(const std::string& path) { _path = path; }
    void addMethod(const std::string& method) { _methods.push_back(method); }
    void setRoot(const std::string& root) { _root = root; }
    void setIndex(const std::string& index) { _index = index; }
    void setAutoindex(bool autoindex) { _autoindex = autoindex; }
    void setRedirect(const std::string& redirect) { _redirect = redirect; }
    void setUploadPath(const std::string& path) { _uploadPath = path; }
    void addCgiExtension(const std::string& ext, const std::string& path) {
        _cgiExtensions[ext] = path;
    }

    // Utility
    bool isMethodAllowed(const std::string& method) const;
    std::string getCgiPath(const std::string& extension) const;
};

#endif
