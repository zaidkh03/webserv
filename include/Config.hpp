#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "webserv.hpp"
#include "Server.hpp"

class Config {
private:
    std::vector<Server> _servers;
    std::string _configFile;

    // Parsing helpers
    void parseServerBlock(std::ifstream& file, Server& server);
    void parseRouteBlock(std::ifstream& file, Route& route);
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);

public:
    Config();
    Config(const std::string& configFile);
    ~Config();

    bool parse();
    const std::vector<Server>& getServers() const { return _servers; }
    void addServer(const Server& server) { _servers.push_back(server); }
};

#endif
