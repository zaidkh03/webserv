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
    bool _uploadEnabled;
    size_t _maxBodySize;
    std::map<std::string, std::string> _cgiExtensions;
    bool _hasMethods;
    bool _hasRoot;
    bool _hasIndex;
    bool _hasAutoindex;
    bool _hasRedirect;
    bool _hasUploadPath;
    bool _hasUploadEnabled;
    bool _hasMaxBodySize;

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
    bool getUploadEnabled() const { return _uploadEnabled; }
    size_t getMaxBodySize() const { return _maxBodySize; }
    const std::map<std::string, std::string>& getCgiExtensions() const { return _cgiExtensions; }
    bool hasMethods() const { return _hasMethods; }
    bool hasRoot() const { return _hasRoot; }
    bool hasIndex() const { return _hasIndex; }
    bool hasAutoindex() const { return _hasAutoindex; }
    bool hasRedirect() const { return _hasRedirect; }
    bool hasUploadPath() const { return _hasUploadPath; }
    bool hasUploadEnabled() const { return _hasUploadEnabled; }
    bool hasMaxBodySize() const { return _hasMaxBodySize; }

    // Setters
    void setPath(const std::string& path) { _path = path; }
    void clearMethods() { _methods.clear(); _hasMethods = true; }
    void addMethod(const std::string& method) { _methods.push_back(method); _hasMethods = true; }
    void setRoot(const std::string& root) { _root = root; _hasRoot = true; }
    void setIndex(const std::string& index) { _index = index; _hasIndex = true; }
    void setAutoindex(bool autoindex) { _autoindex = autoindex; _hasAutoindex = true; }
    void setRedirect(const std::string& redirect) { _redirect = redirect; _hasRedirect = true; }
    void setUploadPath(const std::string& path) { _uploadPath = path; _hasUploadPath = true; }
    void setUploadEnabled(bool enabled) { _uploadEnabled = enabled; _hasUploadEnabled = true; }
    void setMaxBodySize(size_t size) { _maxBodySize = size; _hasMaxBodySize = true; }
    void addCgiExtension(const std::string& ext, const std::string& path) {
        _cgiExtensions[ext] = path;
    }

    // Utility
    bool isMethodAllowed(const std::string& method) const;
    std::string getCgiPath(const std::string& extension) const;
};

#endif
