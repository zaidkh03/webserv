This project has been created as part of the 42 curriculum by zalkhali, galhajaj, zsaleh

# Webserv
## Description

Webserv is a custom HTTP/1.1 web server developed in C++ as part of the 42 curriculum.  
The objective of the project is to understand how real web servers work internally by rebuilding their core behavior from scratch without relying on existing web server frameworks such as Nginx or Apache.

The server is capable of handling multiple client connections simultaneously using non-blocking sockets and an event-driven architecture. It processes HTTP requests, generates appropriate HTTP responses, serves static files, executes CGI scripts, and supports configurable virtual servers through a custom configuration file.

The project focuses heavily on low-level programming concepts such as:

- TCP/IP networking
- Socket programming
- HTTP protocol implementation
- Request parsing
- Event multiplexing (`poll`, `select`, `epoll`, etc.)
- Process and file management
- CGI execution
- Resource and memory management

Main implemented features include:

- HTTP/1.1 support
- GET, POST, and DELETE methods
- Static website serving
- Multiple virtual servers
- Custom error pages
- File uploads
- CGI support
- Autoindex support
- Persistent connections
- Configurable routes and ports

This project provides practical experience with server architecture and demonstrates how modern web servers operate at a low level.

---

## Instructions

### Requirements

Before compiling the project, ensure the following are installed:

- `c++`
- `make`
- Linux or Unix-based operating system

Recommended compiler standard:

```bash
c++ -std=c++98
```

### Clone the Repository

```bash
git clone <repository_url>
cd webserv
```

### Compile the Project

```bash
make
```

Available Makefile rules:

```bash
make        # Compile the project
make clean  # Remove object files
make fclean # Remove object files and executable
make re     # Rebuild the project
```

### Run the Server

Run with a custom configuration file:

```bash
./webserv config/default.conf
```

### Example Configuration File

```conf
server {
    listen 8080;
    server_name localhost;

    root ./www;

    location / {
        index index.html;
    }

    error_page 404 ./errors/404.html;
}
```

### Testing

Open a browser and visit:

```text
http://localhost:8080
```

You can also test the server using `curl`:

#### GET Request

```bash
curl http://localhost:8080
```

#### POST Request

```bash
curl -X POST -d "name=test" http://localhost:8080
```

#### DELETE Request

```bash
curl -X DELETE http://localhost:8080/file.txt
```

---

## Resources

### HTTP & Networking References

- RFC 7230 — Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing
- RFC 7231 — HTTP/1.1 Semantics and Content
- Beej’s Guide to Network Programming
- MDN Web Docs — HTTP Documentation
- Linux man pages:
  - `socket`
  - `bind`
  - `listen`
  - `accept`
  - `poll`
  - `select`
  - `epoll`
  - `fork`
  - `execve`

### C++ References

- The C++ Programming Language — Bjarne Stroustrup
- cppreference.com
- C++98 standard documentation

### Web Server & CGI References

- Nginx documentation
- Apache HTTP Server documentation
- CGI documentation
- POSIX documentation

### AI Usage

AI tools were used as supplementary learning and productivity aids during the development of this project.

AI assistance was mainly used for:

- Understanding HTTP protocol behavior
- Explaining socket programming concepts
- Clarifying CGI execution flow
- Reviewing architecture ideas
- Debugging compilation and runtime issues
- Explaining low-level networking concepts
- Improving code readability and organization
- Assisting with documentation writing
- Generating testing examples and edge cases

All implementation decisions, project architecture, debugging, integration, and final validation were performed manually by the project authors.