#include "../include/CGI.hpp"

CGI::CGI(const std::string& cgiPath, const std::string& scriptPath,
         const Request& request, const Server& server)
    : _cgiPath(cgiPath), _scriptPath(scriptPath), _request(request), _server(server) {
    setupEnvironment(request.getQueryString());
}

CGI::~CGI() {}

void CGI::setupEnvironment(const std::string& queryString) {
    _env["REQUEST_METHOD"] = _request.getMethod();
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["SCRIPT_NAME"] = _request.getPath();
    _env["PATH_INFO"] = _request.getPath();
    _env["QUERY_STRING"] = queryString;
    _env["SERVER_PROTOCOL"] = _request.getVersion();
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    
    std::ostringstream port;
    port << _server.getPort();
    _env["SERVER_PORT"] = port.str();
    _env["SERVER_NAME"] = _server.getServerName();
    
    _env["CONTENT_TYPE"] = _request.getHeader("Content-Type");
    
    std::ostringstream contentLength;
    contentLength << _request.getBody().length();
    _env["CONTENT_LENGTH"] = contentLength.str();
    
    _env["HTTP_HOST"] = _request.getHeader("Host");
    _env["HTTP_USER_AGENT"] = _request.getHeader("User-Agent");
    _env["HTTP_ACCEPT"] = _request.getHeader("Accept");
    _env["HTTP_COOKIE"] = _request.getHeader("Cookie");
}

char** CGI::getEnvArray() {
    char** env = new char*[_env.size() + 1];
    size_t i = 0;
    
    for (std::map<std::string, std::string>::iterator it = _env.begin();
         it != _env.end(); ++it) {
        std::string envVar = it->first + "=" + it->second;
        env[i] = new char[envVar.length() + 1];
        std::strcpy(env[i], envVar.c_str());
        i++;
    }
    env[i] = NULL;
    
    return env;
}

void CGI::freeEnvArray(char** env) {
    for (size_t i = 0; env[i] != NULL; i++)
        delete[] env[i];
    delete[] env;
}

std::string CGI::execute() {
    int pipeIn[2];
    int pipeOut[2];
    
    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
        std::cerr << RED << "CGI: pipe failed" << RESET << std::endl;
        return "";
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << RED << "CGI: fork failed" << RESET << std::endl;
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        return "";
    }
    
    if (pid == 0) {
        // Child process
        close(pipeIn[1]);
        close(pipeOut[0]);
        
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        // Change to script directory
        size_t lastSlash = _scriptPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = _scriptPath.substr(0, lastSlash);
            chdir(dir.c_str());
        }
        
        char** env = getEnvArray();
        char* args[] = {
            const_cast<char*>(_cgiPath.c_str()),
            const_cast<char*>(_scriptPath.c_str()),
            NULL
        };
        
        execve(_cgiPath.c_str(), args, env);
        
        // If execve fails
        std::cerr << RED << "CGI: execve failed" << RESET << std::endl;
        exit(1);
    }
    
    // Parent process
    close(pipeIn[0]);
    close(pipeOut[1]);
    
    // Write request body to CGI
    if (!_request.getBody().empty()) {
        write(pipeIn[1], _request.getBody().c_str(), _request.getBody().length());
    }
    close(pipeIn[1]);
    
    // Read CGI output
    std::string output;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    
    while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytesRead);
    }
    close(pipeOut[0]);
    
    // Wait for child process
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        std::cerr << RED << "CGI: script exited with error" << RESET << std::endl;
        return "";
    }
    
    return output;
}

std::string CGI::executeChunked() {
    // For chunked requests, un-chunk the body first
    // The Request class should handle this
    return execute();
}
