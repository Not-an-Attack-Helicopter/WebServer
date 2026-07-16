/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:35:38 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/30 18:35:39 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Parser.hpp"
#include "../incs/templates.hpp"
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <cstddef>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

static bool isConfigFile(const std::string& filename) {

	size_t dot = filename.rfind('.');

	if (dot == std::string::npos || dot == filename.size() - 1) {
		return false;
	}

	std::string ext = filename.substr(dot + 1);

	return (ext == "conf");
}

static std::string stripInlineComment(std::string line) {

	size_t commentPos = line.find('#');
	if (commentPos != std::string::npos) {
		line = line.substr(0, commentPos);
	}

	return line;

}

static bool isSupportedMethod(const std::string& method) {

	const std::string valid_methods[] = {"GET", "POST", "PUT", "HEAD", "DELETE"};
	const size_t size = arraySize(valid_methods);

	return (std::find(valid_methods, valid_methods + size, method) != valid_methods + size);

}

static bool isSupportedCGIExtension(const std::string& ext) {

	const std::string valid_exts[] = {".py", ".sh"};
	const size_t s = arraySize(valid_exts);

	return (std::find(valid_exts, valid_exts + s, ext) != valid_exts + s);

}

// static bool isRegularFile(const std::string& path) {

// 	// if (access(path.c_str(), F_OK) == -1) {
// 	// 	return false;
// 	// } // redundant

// 	struct stat sb;
// 	if (stat(path.c_str(), &sb) != 0) {
// 		return false;
// 	}

// 	return S_ISREG(sb.st_mode);

// }

// static bool isDirectory(const std::string& path) {

// 	// if (access(path.c_str(), F_OK) == -1) {
// 	// 	return false;
// 	// }

// 	struct stat sb;
// 	if (stat(path.c_str(), &sb) != 0) {
// 		return false;
// 	}

// 	return S_ISDIR(sb.st_mode);

// }

// static bool isReadable(const std::string& path) {

// 	if (!isRegularFile(path)) {
// 		return false;
// 	}

// 	return access(path.c_str(), R_OK) == 0;

// }

static bool isWritable(const std::string& path) {

	// if (!isRegularFile(path)) { // Checking directory!
	// 	return false;
	// }

	return access(path.c_str(), W_OK) == 0;

}

static bool isExecutable(const std::string& path) {

	if (!isRegularFile(path)) {
		return false;
	}

	return /*isRegularFile(path) && */access(path.c_str(), X_OK) == 0;

}

static bool isValidPort(const std::string& port_str) {

	if (port_str.empty() || port_str.size() > 5) {
		return false;
	}

	std::istringstream iss(port_str);
	int port;

	return (iss >> port).eof() && 0 < port && port <= 65535;

}

static bool isValidIPAddress(const std::string& ip) {

	std::istringstream iss(ip);
	std::string token;
	int count = 0;

	while (std::getline(iss, token, '.')) {

		if (token.empty() || token.size() > 3) {
			return false;
		}

		for (size_t i = 0; i < token.size(); ++i) {

			if (!std::isdigit(static_cast<unsigned char>(token[i]))) {
				return false;
			}

		}

		int num = std::atoi(token.c_str());
		if (num < 0 || num > 255) {
			return false;
		}

		count++;

	}

	return count == 4;

}

static bool isValidBodySize(const int size) {

	return size >= 0;

}

// static bool isValidErrorCode(const int code) {
//
// 	return code >= 400 && code < 600;
//
// }

static bool isAlreadyInUse(const std::vector<Config>& config, const std::string& host, int port) {

	for (size_t i = 0; i < config.size(); ++i) {

		if (config[i].port == port && config[i].host == host) {
			return true;
		}

	}

	return false;

}

static std::string extractDirectiveKey(const std::string& line) {

	size_t space = line.find(' ');
	if (space == std::string::npos) {
		return line;
	}

	return line.substr(0, space);

}

static std::string extractDirectiveValue(const std::string& line) {

	size_t space = line.find(' ');
	if (space == std::string::npos) {
		return "";
	}

	std::string val = line.substr(space + 1);
	if (!val.empty() && val[val.size() - 1] == ';') {
		val = val.substr(0, val.size() - 1);
	}

	return trim(val);

}

static void extractLocationPath(const std::string& header, Location& loc) {

	size_t first_space = header.find(' ');
	size_t last_space  = header.rfind(' ');

	if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space) {
		loc.path = trim(header.substr(first_space + 1, last_space - first_space - 1));

	} else {
		loc.path = "/";
	}

	if (loc.path.empty()) {
		throw std::runtime_error("parse error: location directive requires a value");
	}

	if (loc.path.size() > 1 && loc.path[0] != '/') {
		throw std::runtime_error("parse error: location path must start with '/': " + loc.path);
	}

	if (loc.path.size() > 1 && loc.path[loc.path.size() - 1] == '/') {
		throw std::runtime_error("parse error: location path must not end with '/': " + loc.path);
	}

	return;

}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Parser& Parser::instance(void) {
	static Parser instance;
	return instance;
}

const std::vector<Config>& Parser::getAllConfigs() const {
	return _configs;
}

const Config& Parser::getConfig(size_t index) const {
	return _configs[index];
}

size_t Parser::getNumConfigs(void) const {
	return _configs.size();
}

void Parser::scan(const std::string& config_file) {

	if(!isConfigFile(config_file)) {
		throw std::runtime_error("invalid file extension: " + config_file);
	}

	std::ifstream file(config_file.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("could not open file: " + config_file);
	}

	bool foundServer = false;

	std::string line;
	while (std::getline(file, line)) {

		std::string trimmed = trim(line);

		// Skip empty lines and comments
		if (trimmed.empty() || trimmed[0] == '#') {
			continue;
		}

		// Strip line from inline comments
		std::string stripped = stripInlineComment(trimmed);
		trimmed = trim(stripped);

		if (trimmed == "server {") {
			_parseServerBlock(file);
			foundServer = true;

		} else if (trimmed == "server") {
			throw std::runtime_error("parse error: 'server' directive requires '{'");

		} else {
			throw std::runtime_error("parse error: unexpected directive outside server block: " + trimmed);
		}

	}

	if (!foundServer) {
		throw std::runtime_error("parse error: config file must contain at least one server block");
	}

	file.close();

	return;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Parser::Parser(void) {
	log.debug("Parser Constructor called");
	return;
};

/*	@brief Copy Constructor	*/
Parser::Parser(const Parser& other) : _configs(other._configs) {
	log.debug("Parser Copy Constructor called");
	return;
};

/*	@brief Copy Assignment Operator	*/
Parser& Parser::operator=(const Parser& other) {
	log.debug("Parser Copy Assignment Operator called");
	if (this != &other) {
		this->_configs = other._configs;
	}
	return *this;
};

/*	@brief Deconstructor	*/
Parser::~Parser() {
	log.debug("Parser Deconstructor called");
	return;
};

Parser::locationDirectiveHandlerMap Parser::_initLocationDirectiveHandlerMap(void) {

	locationDirectiveHandlerMap handlers;

	handlers["root"] = &Parser::_handleRoot;
	handlers["redirect"] = &Parser::_handleRedirect;
	handlers["allow_methods"] = &Parser::_handleAllowedMethods;
	handlers["autoindex"] = &Parser::_handleAutoindex;
	handlers["index"] = &Parser::_handleIndexFile;
	handlers["upload_dir"] = &Parser::_handleUploadDirectory;
	// handlers["cgi_ext"] = &Parser::_handleCGIExt;
	// handlers["cgi_path"] = &Parser::_handleCGIPath;
	handlers["interpreter"] = &Parser::_handleInterpreter;
	handlers["error_page"] = &Parser::_handleErrorPage;

	return handlers;

}

Parser::serverDirectiveHandlerMap Parser::_initServerDirectiveHandlerMap(void) {

	serverDirectiveHandlerMap handlers;

	handlers["listen"] = &Parser::_handleListen;
	handlers["host"] = &Parser::_handleHost;
	handlers["server_name"] = &Parser::_handleServerNames;
	handlers["root"] = &Parser::_handleRoot;
	handlers["index"] = &Parser::_handleIndexFile;
	handlers["error_page"] = &Parser::_handleErrorPage;
	handlers["client_max_body_size"] = &Parser::_handleClientMaxBodySize;

	return handlers;
}

void Parser::_parseLocationBlock(std::ifstream& config_file,
								 Config& config, Location& loc) {

	static const locationDirectiveHandlerMap handlers = _initLocationDirectiveHandlerMap();

	std::string line;
	while (std::getline(config_file, line)) {

		std::string trimmed = trim(line);

		// Skip empty lines and comments
		if (trimmed.empty() || trimmed[0] == '#') {
			continue;
		}

		// End of block
		if (trimmed == "}") {

			// // Block is complete — now finalize and validate
			// if (loc.cgi_extensions.size() != loc.cgi_paths.size()) {
			// 	throw std::runtime_error("parse error: cgi_ext and cgi_path count mismatch in location '" + loc.path + "'");
			// }

			// Check for location root: if empty, substitute server root
			if (loc.root.empty()) {
				loc.root = config.root;
			}

			if (loc.index_files.empty()) {
				loc.index_files = config.index_files;
			}

			if (loc.error_pages.empty()) {
				loc.error_pages = config.error_pages;
			}

			// Check for duplicate paths
			for (size_t i = 0; i < config.locations.size(); ++i) {

				if (config.locations[i].path == loc.path) {
					throw std::runtime_error("parse error: duplicate location path '" + loc.path + "'");
				}

			}

			config.locations.push_back(loc);

			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, loc);

		} else {
			throw std::runtime_error("parse error: unknown directive in location block: " + key);
		}

	}

	// If we exit the loop without finding }, throw
	throw std::runtime_error("parse error: unclosed location block '" + loc.path + "' (missing '}')");

	return;

}

void Parser::_parseServerBlock(std::ifstream& config_file_stream) {

	Config server_block;
	server_block.port = 80;
	server_block.client_max_body_size = 1048576;

	static const serverDirectiveHandlerMap handlers = _initServerDirectiveHandlerMap();

	std::string line;
	while (std::getline(config_file_stream, line)) {

		std::string trimmed = trim(line);

		// Skip empty lines and comments
		if (trimmed.empty() || trimmed[0] == '#') {
			continue;
		}

		// End of block
		if (trimmed == "}") {

			// Block is complete — now finalize and validate
			if (server_block.host.empty()) {
				server_block.host = "0.0.0.0";
			}

			// Check for duplicate hosts
			if (isAlreadyInUse(_configs, server_block.host, server_block.port)) {
				std::ostringstream oss;
				oss	<< "parse error: host " << server_block.host
					<< " port " << server_block.port
					<< " is already in use by another server block";
				throw std::runtime_error(oss.str());
			}

			_configs.push_back(server_block);

			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, server_block);

		} else if (key == "location") {

			Location location_block;
			location_block.autoindex = false;

			// Read the location header line
			extractLocationPath(trimmed, location_block);

			// Read the location block
			_parseLocationBlock(config_file_stream, server_block, location_block);

			continue;

		} else {
			throw std::runtime_error("parse error: unknown directive in server block: " + key);
		}

	}

	// If we get here, block was never closed
	throw std::runtime_error("parse error: unclosed server block (missing '}')");

	return;

}

// void Parser::_handleRoot(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: root directive requires a value");
// 	}

// 	if (val[0] != '/') {
// 		throw std::runtime_error("parse error: root path must start with '/': " + val);
// 	}

// 	if (val[val.size() - 1] == '/') {
// 		throw std::runtime_error("parse error: root path must not end with '/': " + val);
// 	}

// 	loc.root = val;

// 	return;

// }

void Parser::_handleRedirect(const std::string& val, Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: redirect directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: redirect value must start with '/': " + val);
	}

	if (val[val.size() - 1] == '/') {
		throw std::runtime_error("parse error: redirect value must not end with '/': " + val);
	}

	loc.redirect = val;

	return;

}

void Parser::_handleAllowedMethods(const std::string& val, Location& loc) {

	if (!loc.methods.empty())
		log.debug(loc.methods.back());

	std::istringstream iss(val);
	std::string method;

	while (iss >> method) {

		if (!isSupportedMethod(method)) {
			throw std::runtime_error("parse error: invalid HTTP method: " + method);
		}

		loc.methods.push_back(method);

	}

	return;

}

void Parser::_handleAutoindex(const std::string& val, Location& loc) {

	if (val != "on" && val != "off") {
		throw std::runtime_error("parse error: autoindex must be 'on' or 'off', got: " + val);
	}

	loc.autoindex = (val == "on");

	return;

}

// void Parser::_handleIndexFile(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: index_files directive requires at least one file");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		// if (val.empty()) {
// 		// 	throw std::runtime_error("parse error: index file directive requires a value");
// 		// } // redundant, see "if (!(iss >> path))"

// 		if (val[0] == '/') {
// 			throw std::runtime_error("parse error: index file path must not start with '/': " + val);
// 		}

// 		if (val[val.size() - 1] == '/') {
// 			throw std::runtime_error("parse error: index file path must not end with '/': " + val);
// 		}

// 		if (!isReadable(loc.root + "/" + path)) {
// 			throw std::runtime_error("parse error: no read access: " + path);
// 		}

// 		loc.index_files.push_back(path);

// 	}

// 	return;

// }

void Parser::_handleUploadDirectory(const std::string& val, Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: upload_dir directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: upload directory path must start with '/': " + val);
	}

	if (val[val.size() - 1] == '/') {
		throw std::runtime_error("parse error: upload directory path must not end with '/': " + val);
	}

	if (!isWritable(val)) {
		throw std::runtime_error("parse error: no write access (upload directory): " + val);
	}

	loc.upload_dir = val;

	return;

}

// void Parser::_handleCGIExt(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: cgi_ext directive requires at least one extension");
// 	}

// 	std::istringstream iss(val);
// 	std::string ext;

// 	while (iss >> ext) {

// 		if (!isSupportedCGIExtension(ext)) {
// 			throw std::runtime_error("parse error: invalid CGI extension: " + ext);
// 		}

// 		loc.cgi_extensions.push_back(ext);

// 	}

// 	return;

// }

// void Parser::_handleCGIPath(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: cgi_path directive requires at least one path");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		if (path[0] != '/') {
// 			throw std::runtime_error("parse error: CGI path must start with '/': " + path);
// 		}

// 		if (path[path.size() - 1] == '/') {
// 			throw std::runtime_error("parse error: CGI path must not end with '/': " + path);
// 		}

// 		if (!isExecutable(path)) {
// 			throw std::runtime_error("parse error: invalid CGI path (not executable): " + path);
// 		}

// 		loc.cgi_paths.push_back(path);

// 	}

// 	return;

// }

void Parser::_handleInterpreter(const std::string& val, Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: interpreter directive requires extension and path");
	}

	std::istringstream iss(val);
	std::string ext;
	std::string path;

	if (!(iss >> ext >> path)) {
		throw std::runtime_error("parse error: interpreter requires both extension and path");
	}

	if (!isSupportedCGIExtension(ext)) {
		throw std::runtime_error("parse error: unsupported CGI extension: " + ext);
	}

	// if (path.empty()) {
	// 	throw std::runtime_error("parse error: interpreter directive requires a value");
	// } // redundant, see "if (!(iss >> ext >> path))"

	if (path[0] != '/') {
		throw std::runtime_error("parse error: interpreter path must start with '/': " + path);
	}

	if (path[path.size() - 1] == '/') {
		throw std::runtime_error("parse error: interpreter path must not end with '/': " + path);
	}

	if (!isExecutable(path)) {
		throw std::runtime_error("parse error: interpreter path not executable: " + path);
	}

	if (loc.interpreters.count(ext) > 0) {
		throw std::runtime_error("parse error: CGI extension '" + ext
									+ "' already mapped in location '" + loc.path + "'");
	}

	loc.interpreters[ext] = path;

	return;

}

// void Parser::_handleErrorPage(const std::string& val, Location& loc) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: error_page directive requires code and path");
// 	}
//
// 	std::istringstream iss(val);
// 	std::string code_str;
// 	std::string path;
//
// 	if (!(iss >> code_str >> path)) {
// 		throw std::runtime_error("parse error: error_page directive requires both code and path");
// 	}
//
// 	int code = stringToInt(code_str);
//
// 	if (!isValidErrorCode(code)) {
// 		throw std::runtime_error("parse error: invalid error code: " + code_str);
// 	}
//
// 	// if (path.empty()) {
// 	// 	throw std::runtime_error("parse error: error page directive requires a value");
// 	// } // redundant, see "if (!(iss >> code_str >> path))"
//
// 	if (path[0] == '/') {
// 		throw std::runtime_error("parse error: error page file path must not start with '/': " + path);
// 	}
//
// 	if (path[path.size() - 1] == '/') {
// 		throw std::runtime_error("parse error: error page file path must not end with '/': " + path);
// 	}
//
// 	if (!isReadable(path)) {
// 		throw std::runtime_error("parse error: no read access: " + path);
// 	}
//
// 	loc.error_pages[code] = path;
//
// 	return;
//
// }

void Parser::_handleListen(const std::string& val, Config& config) {

	if (!isValidPort(val)) {
		throw std::runtime_error("parse error: invalid port value: " + val);
	}

	config.port = stringToUnsignedShort(val);

	return;

}

void Parser::_handleHost(const std::string& val, Config& config) {

	if (!isValidIPAddress(val)) {
		throw std::runtime_error("parse error: invalid host IP: " + val);
	}

	config.host = val;

	return;

}

void Parser::_handleServerNames(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: server_name directive requires at least one name");
	}

	std::istringstream iss(val);
	std::string name;

	while (iss >> name) {

		if (name.empty()) {
			throw std::runtime_error("parse error: empty server_name");
		}

		config.server_names.push_back(name);

	}

	return;

}

// void Parser::_handleRoot(const std::string& val, Config& config) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: root directive requires a value");
// 	}

// 	if (val[0] != '/') {
// 		throw std::runtime_error("parse error: root path must start with '/': " + val);
// 	}

// 	if (val[val.size() - 1] == '/') {
// 		throw std::runtime_error("parse error: root path must not end with '/': " + val);
// 	}

// 	config.root = val;

// 	return;

// }

// void Parser::_handleIndexFile(const std::string& val, Config& config) {

// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: index_files directive requires at least one file");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		// if (val.empty()) {
// 		// 	throw std::runtime_error("parse error: index file directive requires a value");
// 		// } // redundant, see "if (!(iss >> path))"

// 		if (val[0] == '/') {
// 			throw std::runtime_error("parse error: index file path must not start with '/': " + val);
// 		}

// 		if (val[val.size() - 1] == '/') {
// 			throw std::runtime_error("parse error: index file path must not end with '/': " + val);
// 		}

// 		if (!isReadable(config.root + "/" + path)) {
// 			throw std::runtime_error("parse error: no read access: " + path);
// 		}

// 		config.index_files.push_back(path);

// 	}

// 	return;

// }

// void Parser::_handleErrorPage(const std::string& val, Config& config) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("parse error: error_page directive requires code and path");
// 	}
//
// 	std::istringstream iss(val);
// 	std::string code_str;
// 	std::string path;
//
// 	if (!(iss >> code_str >> path)) {
// 		throw std::runtime_error("parse error: error_page requires both status code and path");
// 	}
//
// 	int code = stringToInt(code_str);
//
// 	if (!isValidErrorCode(code)) {
// 		throw std::runtime_error("parse error: invalid error code: " + code_str);
// 	}
//
// 	// if (path.empty()) {
// 	// 	throw std::runtime_error("parse error: error page directive requires a value");
// 	// } // redundant, see "if (!(iss >> code_str >> path))"
//
// 	if (path[0] == '/') {
// 		throw std::runtime_error("parse error: error page file path must not start with '/': " + path);
// 	}
//
// 	if (path[path.size() - 1] == '/') {
// 		throw std::runtime_error("parse error: error page file path must not end with '/': " + path);
// 	}
//
// 	if (!isReadable(path)) {
// 		throw std::runtime_error("parse error: no read access: " + path);
// 	}
//
// 	config.error_pages[code] = path;
//
// 	return;
//
// }

void Parser::_handleClientMaxBodySize(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: client_max_body_size requires a value");
	}

	size_t size = stringToSize(val);

	if (!isValidBodySize(size)) {
		throw std::runtime_error("parse error: invalid client_max_body_size: " + val);
	}

	config.client_max_body_size = size;

	return;

}
