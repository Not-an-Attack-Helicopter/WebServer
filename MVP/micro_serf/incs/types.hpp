#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <map>

// Configuration
struct LocationConfig {
	std::string						path;
	std::vector<std::string>		methods;       // GET, POST, DELETE
	std::string						root;
	std::string						index;
	std::string						redirect;       // return 301 /url
	bool							autoindex;
	std::string						upload_dir;
	std::vector<std::string>		cgi_extension;  // .py, .sh
	std::vector<std::string>		cgi_path;      // /usr/bin/python3, /bin/bash
};

struct ServerConfig {
	std::string						host;
	int								port;
	std::string						root;
	std::string						index;
	std::vector<std::string>		server_names;
	size_t							client_max_body_size;
	std::map<int, std::string>		error_pages;   // 404 -> /404.html
	std::vector<LocationConfig>		locations;
};

// typedef std::vector<ServerConfig> configs_t;

// Client
enum ParseState {
	PS_REQUEST_LINE,
	PS_READING_HEADERS,
	PS_READING_BODY,
	PS_COMPLETE,
	PS_ERROR
};

// DEBUG
enum AdminCommand {
	STOP = 2000
};
// DEBUG
