#include "../include/Config.hpp"

Config::Config() : _configFile("config/default.conf") {}

Config::Config(const std::string& configFile) : _configFile(configFile) {}

Config::~Config() {}

std::string Config::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Config::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        token = trim(token);
        if (!token.empty())
            tokens.push_back(token);
    }
    return tokens;
}

void Config::parseRouteBlock(std::ifstream& file, Route& route) {
    std::string line;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
            
        if (line == "}")
            break;
            
        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.empty())
            continue;
            
        if (tokens[0] == "methods" && tokens.size() >= 2) {
            for (size_t i = 1; i < tokens.size(); i++) {
                std::string method = tokens[i];
                if (method[method.length() - 1] == ';')
                    method = method.substr(0, method.length() - 1);
                route.addMethod(method);
            }
        }
        else if (tokens[0] == "root" && tokens.size() >= 2) {
            std::string root = tokens[1];
            if (root[root.length() - 1] == ';')
                root = root.substr(0, root.length() - 1);
            route.setRoot(root);
        }
        else if (tokens[0] == "index" && tokens.size() >= 2) {
            std::string index = tokens[1];
            if (index[index.length() - 1] == ';')
                index = index.substr(0, index.length() - 1);
            route.setIndex(index);
        }
        else if (tokens[0] == "autoindex" && tokens.size() >= 2) {
            std::string val = tokens[1];
            if (val[val.length() - 1] == ';')
                val = val.substr(0, val.length() - 1);
            route.setAutoindex(val == "on" || val == "true");
        }
        else if (tokens[0] == "redirect" && tokens.size() >= 2) {
            std::string redir = tokens[1];
            if (redir[redir.length() - 1] == ';')
                redir = redir.substr(0, redir.length() - 1);
            route.setRedirect(redir);
        }
        else if (tokens[0] == "upload_path" && tokens.size() >= 2) {
            std::string path = tokens[1];
            if (path[path.length() - 1] == ';')
                path = path.substr(0, path.length() - 1);
            route.setUploadPath(path);
        }
        else if (tokens[0] == "cgi" && tokens.size() >= 3) {
            std::string ext = tokens[1];
            std::string path = tokens[2];
            if (path[path.length() - 1] == ';')
                path = path.substr(0, path.length() - 1);
            route.addCgiExtension(ext, path);
        }
    }
}

void Config::parseServerBlock(std::ifstream& file, Server& server) {
    std::string line;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
            
        if (line == "}")
            break;
            
        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.empty())
            continue;
            
        if (tokens[0] == "listen" && tokens.size() >= 2) {
            std::string listen = tokens[1];
            if (listen[listen.length() - 1] == ';')
                listen = listen.substr(0, listen.length() - 1);
                
            size_t colon = listen.find(':');
            if (colon != std::string::npos) {
                server.setHost(listen.substr(0, colon));
                server.setPort(std::atoi(listen.substr(colon + 1).c_str()));
            } else {
                server.setPort(std::atoi(listen.c_str()));
            }
        }
        else if (tokens[0] == "server_name" && tokens.size() >= 2) {
            std::string name = tokens[1];
            if (name[name.length() - 1] == ';')
                name = name.substr(0, name.length() - 1);
            server.setServerName(name);
        }
        else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2) {
            std::string size = tokens[1];
            if (size[size.length() - 1] == ';')
                size = size.substr(0, size.length() - 1);
            
            size_t value = std::atoi(size.c_str());
            if (size.find('M') != std::string::npos || size.find('m') != std::string::npos)
                value *= 1048576;
            else if (size.find('K') != std::string::npos || size.find('k') != std::string::npos)
                value *= 1024;
            server.setClientMaxBodySize(value);
        }
        else if (tokens[0] == "error_page" && tokens.size() >= 3) {
            int code = std::atoi(tokens[1].c_str());
            std::string path = tokens[2];
            if (path[path.length() - 1] == ';')
                path = path.substr(0, path.length() - 1);
            server.addErrorPage(code, path);
        }
        else if (tokens[0] == "location" && tokens.size() >= 2) {
            std::string path = tokens[1];
            if (path[path.length() - 1] == '{')
                path = path.substr(0, path.length() - 1);
            path = trim(path);
            
            // Read opening brace if not on same line
            if (line.find('{') == std::string::npos) {
                while (std::getline(file, line)) {
                    line = trim(line);
                    if (line == "{")
                        break;
                }
            }
            
            Route route(path);
            parseRouteBlock(file, route);
            server.addRoute(route);
        }
    }
}

bool Config::parse() {
    std::ifstream file(_configFile.c_str());
    if (!file.is_open()) {
        std::cerr << RED << "Error: Cannot open config file: " << _configFile << RESET << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
            
        if (line == "server" || line == "server {") {
            // Read opening brace if not on same line
            if (line.find('{') == std::string::npos) {
                while (std::getline(file, line)) {
                    line = trim(line);
                    if (line == "{")
                        break;
                }
            }
            
            Server server;
            parseServerBlock(file, server);
            _servers.push_back(server);
        }
    }

    file.close();
    
    if (_servers.empty()) {
        std::cerr << RED << "Error: No server blocks found in config" << RESET << std::endl;
        return false;
    }

    std::cout << GREEN << "Parsed " << _servers.size() << " server block(s)" << RESET << std::endl;
    return true;
}
