#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstddef>


// Configuration
struct LocationConfig {
    std::string             path;
    std::vector<std::string> methods;       // GET, POST, DELETE
    std::string             root;
    std::string             index;
    std::string             redirect;       // return 301 /url
    bool                    autoindex;
    std::string             upload_dir;
    std::string             cgi_extension;  // .py, .php
    std::vector<std::string> cgi_path;      // /usr/bin/python3, /bin/bash
};

struct ServerConfig {
    std::string                  host;
    int                          port;
    std::vector<std::string>     server_names;
    size_t                       client_max_body_size;
    std::map<int, std::string>   error_pages;   // 404 -> /404.html
    std::vector<LocationConfig>  locations;
};

struct Config {
    std::vector<ServerConfig> servers;
};

// Client

enum ParseState {
    PS_REQUEST_LINE,
    PS_READING_HEADERS,
    PS_READING_BODY,
    PS_COMPLETE,
    PS_ERROR
};
