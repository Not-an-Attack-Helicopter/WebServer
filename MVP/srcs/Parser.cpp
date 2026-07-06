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
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/templates.hpp"
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

static bool isExecutable(const std::string& path) {

	struct stat buffer;

	if (stat(path.c_str(), &buffer) != 0) {
		return false;
	}

	return S_ISREG(buffer.st_mode) && access(path.c_str(), X_OK) == 0;

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

static bool isValidErrorCode(const int code) {

	return code >= 400 && code < 600;

}

static bool isAlreadyInUse(const std::vector<Config>& config, const std::string& host, int port) {

	for (size_t i = 0; i < config.size(); ++i) {

		if (config[i].port == port && config[i].host == host) {
			return true;
		}

	}

	return false;

}

static void extractLocationPath(const std::string& header, LocationConfig& loc) {

	size_t first_space = header.find(' ');
	size_t last_space  = header.rfind(' ');

	if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space) {
		loc.path = trim(header.substr(first_space + 1, last_space - first_space - 1));

	} else {
		loc.path = "/";
	}

	if (loc.path.empty() || loc.path[0] != '/') {
		throw std::runtime_error("parse error: invalid location path (must start with '/'): " + loc.path);
	}

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

/*	@brief Destructor	*/
Parser::~Parser() {
	log.debug("Parser Destructor called");
	return;
};

Parser::locationDirectiveHandlerMap Parser::_initLocationDirectiveHandlerMap(void) {

	locationDirectiveHandlerMap handlers;

	handlers["allow_methods"] = &Parser::_handleAllowedMethods;
	handlers["autoindex"] = &Parser::_handleAutoindex;
	handlers["index"] = &Parser::_handleIndex;
	handlers["root"] = &Parser::_handleRoot;
	handlers["return"] = &Parser::_handleReturn;
	handlers["upload_dir"] = &Parser::_handleUploadDir;
	handlers["cgi_ext"] = &Parser::_handleCGIExt;
	handlers["cgi_path"] = &Parser::_handleCGIPath;

	return handlers;

}

Parser::serverDirectiveHandlerMap Parser::_initServerDirectiveHandlerMap(void) {

	serverDirectiveHandlerMap handlers;

	handlers["listen"] = &Parser::_handleListen;
	handlers["host"] = &Parser::_handleHost;
	handlers["server_name"] = &Parser::_handleServerNames;
	handlers["client_max_body_size"] = &Parser::_handleClientMaxBodySize;
	handlers["error_page"] = &Parser::_handleErrorPage;
	handlers["root"] = &Parser::_handleServerRoot;
	handlers["index"] = &Parser::_handleServerIndex;

	return handlers;
}

void Parser::_handleAllowedMethods(const std::string& val, LocationConfig& loc) {

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

}

void Parser::_handleAutoindex(const std::string& val, LocationConfig& loc) {

	if (val != "on" && val != "off") {
		throw std::runtime_error("parse error: autoindex must be 'on' or 'off', got: " + val);
	}

	loc.autoindex = (val == "on");

}

void Parser::_handleIndex(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: index directive requires a value");
	}

	loc.index = val;

}

void Parser::_handleRoot(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: root directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: root path must start with '/': " + val);
	}

	loc.root = val;

}

void Parser::_handleReturn(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: return directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: return value must start with '/': " + val);
	}

	loc.redirect = val;

}

void Parser::_handleUploadDir(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: upload_dir directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: upload_dir path must start with '/': " + val);
	}

	loc.upload_dir = val;

}

void Parser::_handleCGIExt(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: cgi_ext directive requires at least one extension");
	}

	std::istringstream iss(val);
	std::string ext;

	while (iss >> ext) {

		if (!isSupportedCGIExtension(ext)) {
			throw std::runtime_error("parse error: invalid CGI extension: " + ext);
		}

		loc.cgi_extensions.push_back(ext);

	}

}

void Parser::_handleCGIPath(const std::string& val, LocationConfig& loc) {

	if (val.empty()) {
		throw std::runtime_error("parse error: cgi_path directive requires at least one path");
	}

	std::istringstream iss(val);
	std::string path;

	while (iss >> path) {

		if (!isExecutable(path)) {
			throw std::runtime_error("parse error: invalid CGI path (not executable): " + path);
		}

		loc.cgi_paths.push_back(path);

	}

}

void Parser::_parseLocationBlock(std::ifstream& config_file, Config& config, LocationConfig& loc) {

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

			// Block is complete — now finalize and validate
			if (loc.cgi_extensions.size() != loc.cgi_paths.size()) {
				throw std::runtime_error("parse error: cgi_ext and cgi_path \
										count mismatch in location '" + loc.path + "'");
			}

			// Check for duplicate paths
			for (size_t i = 0; i < config.locations.size(); ++i) {

				if (config.locations[i].path == loc.path) {
					throw std::runtime_error("parse error: \
											duplicate location path '" + loc.path + "'");
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
			throw std::runtime_error("parse error: \
									unknown directive in location block: " + key);
		}

	}

	// If we exit the loop without finding }, throw
	throw std::runtime_error("parse error: \
							unclosed location block '" + loc.path + "' (missing '}')");

}

void Parser::_handleListen(const std::string& val, Config& config) {

	if (!isValidPort(val)) {
		throw std::runtime_error("parse error: invalid port value: " + val);
	}

	config.port = stringToUnsignedShort(val);

}

void Parser::_handleHost(const std::string& val, Config& config) {

	if (!isValidIPAddress(val)) {
		throw std::runtime_error("parse error: invalid host IP: " + val);
	}

	config.host = val;

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

}

void Parser::_handleClientMaxBodySize(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: client_max_body_size requires a value");
	}

	size_t size = stringToSize(val);

	if (!isValidBodySize(size)) {
		throw std::runtime_error("parse error: invalid client_max_body_size: " + val);
	}

	config.client_max_body_size = size;

}

void Parser::_handleErrorPage(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: error_page directive requires code and path");
	}

	std::istringstream iss(val);
	std::string code_str;
	std::string path;

	if (!(iss >> code_str >> path)) {
		throw std::runtime_error("parse error: error_page requires both status code and path");
	}

	int code = stringToInt(code_str);

	if (!isValidErrorCode(code)) {
		throw std::runtime_error("parse error: invalid error code: " + code_str);
	}

	if (path.empty()) {
		throw std::runtime_error("parse error: error_page path cannot be empty");
	}

	config.error_pages[code] = path;

}

void Parser::_handleServerRoot(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: root directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("parse error: root path must start with '/': " + val);
	}

	config.root = val;
}

void Parser::_handleServerIndex(const std::string& val, Config& config) {

	if (val.empty()) {
		throw std::runtime_error("parse error: index directive requires a value");
	}

	config.index = val;

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
				oss << "parse error: host " << server_block.host << " port " << server_block.port
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

			LocationConfig location_block;
			location_block.autoindex = false;

			// Read the location header line
			extractLocationPath(trimmed, location_block);

			// Read the location block
			_parseLocationBlock(config_file_stream, server_block, location_block);

			continue;

		} else {
			throw std::runtime_error("parse error: \
									unknown directive in server block: " + key);
		}

	}

	// If we get here, block was never closed
	throw std::runtime_error("parse error: \
							unclosed server block (missing '}')");

}
