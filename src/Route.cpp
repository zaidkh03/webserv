#include "../include/Route.hpp"

Route::Route() : _path("/"), _autoindex(false) {
    _methods.push_back("GET");
}

Route::Route(const std::string& path) : _path(path), _autoindex(false) {
    _methods.push_back("GET");
}

Route::~Route() {}

bool Route::isMethodAllowed(const std::string& method) const {
    for (size_t i = 0; i < _methods.size(); i++) {
        if (_methods[i] == method)
            return true;
    }
    return false;
}

std::string Route::getCgiPath(const std::string& extension) const {
    std::map<std::string, std::string>::const_iterator it = _cgiExtensions.find(extension);
    if (it != _cgiExtensions.end())
        return it->second;
    return "";
}
