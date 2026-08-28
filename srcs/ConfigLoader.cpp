/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLoader.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:35:38 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 21:52:26 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/ConfigLoader.hpp"
#include "../incs/Dispatcher.hpp"
#include "../incs/templates.hpp"
// #include "../incs/constexpr.hpp"
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
// #include <sstream>
#include <vector>
#include <cstddef>
// #include <climits>
#include <cctype>

// Config file parsing helpers
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

// static bool isSupportedMethod(const std::string& method) {
//
// 	const std::string valid_methods[METHOD_COUNT] = {"GET", "HEAD", "DELETE", "POST", "PUT"};
// 	// const size_t size = arraySize(valid_methods);
// 	if (std::find(valid_methods, valid_methods + METHOD_COUNT, method) != valid_methods + METHOD_COUNT) {
// 		return true;
// 	} else {
// 		return false;
// 	}
// 	// return (std::find(valid_methods, valid_methods + METHOD_COUNT, method) != valid_methods + METHOD_COUNT);
// }

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

// static bool isTchar(char c) {
// 	return std::isalnum(static_cast<unsigned char>(c)) ||
// 	c == '!' || c == '#' || c == '$' ||
// 	c == '%' || c == '&' || c == '\'' ||
// 	c == '*' || c == '+' || c == '-' ||
// 	c == '.' || c == '^' || c == '_' ||
// 	c == '`' || c == '|' || c == '~';
// }

static bool isReadable(const std::string& path) {

	if (!isRegularFile(path)) return false;

	int fd = open(path.c_str(), O_RDONLY);
	if (fd == -1) return false;
	close(fd);
	return true;

}

// static bool isWritable(const std::string& path) {
//
// 	// if (!isRegularFile(path)) { // checking directory!
// 	// 	return false;
// 	// }
//
// 	return access(path.c_str(), W_OK) == 0;
//
// }

static bool isExecutable(const std::string& path) {

	if (!isRegularFile(path)) return false;

	return /*isRegularFile(path) && */access(path.c_str(), X_OK) == 0;

}

static bool isValidPort(const std::string& port_str) {

	if (port_str.empty() || port_str.size() > 5) {
		return false;
	}

	std::istringstream iss(port_str);
	int port;
	char c;

	return (iss >> port) && !(iss >> c) && 0 < port && port <= 65535;

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

// static bool isValidBodySize(const int size) {
//
// 	return size >= 0;
//
// }

// static bool isValidErrorCode(const int code) {
//
// 	return code >= 400 && code < 600;
//
// }

static bool isDuplicateSocket(const std::vector<Config::Socket>& sockets,
							  const std::string& address, int port) {

	for (size_t i = 0; i < sockets.size(); ++i) {
		if (sockets[i].port == port && sockets[i].address == address) {
			return true;
		}
	}
	return false;

}

static bool isDuplicateDomain(const std::vector<Config::Domain>& domains,
							  std::string& name) {

	for (size_t i = 0; i < domains.size(); ++i) {
		for (size_t j = 0; j < domains[i].names.size(); ++j) {
			if (domains[i].names[j] == name) {
				return true;
			}
		}
	}
	return false;
}

static bool isDuplicateLocation(const std::vector<Config::Location>& locations,
								const std::string& path) {

	for (size_t i = 0; i < locations.size(); ++i) {
		if (locations[i].path == path) {
			return true;
		}
	}
	return false;
}
/*
static int createFile(const Config::Location& location,
					  const std::string& path,
					  HTTPRequest& request) {

	log.debug("absolute path: " + path);

	int fd = -1;
	unsigned short count = 0;
	std::time_t timestamp = std::time(NULL);
	do {
		std::string unique_id = i2a(timestamp) + "-" + randomHexString(8);
		std::string file_path = path + location.upload_dir + "/.upload_" + unique_id + ".tmp";
		request.body.file_path = file_path;
		log.debug("file_path: " + file_path);
		fd = open(file_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
	} while (fd == -1 && errno == EEXIST && ++count < 11);

	return fd;

}
	request.body.file_fd = createFile(location, path, request);
*/

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

static void extractDomainNames(const std::string& header, Config::Domain& dom) {

	if (header.empty()) {
		throw std::runtime_error("config error: domain directive requires at least one name");
	}

	size_t first_pos = header.find(' ');
	size_t last_pos  = header.rfind(' ');
	std::string names;
	if (first_pos != std::string::npos && last_pos != std::string::npos && first_pos != last_pos) {
		names = trim(header.substr(first_pos + 1, last_pos - first_pos - 1));
	}

	std::ostringstream oss;
	std::istringstream iss(names);
	std::string name;

	while (iss >> name) {

		if (name.empty()) {
			throw std::runtime_error("config error: empty domain name");
		}

		if (name.size() > 253) {
			oss << "config error: domain name too long (RFC 1035): "
				<< name.substr(0, 12) << "..." << std::endl;
			throw std::runtime_error(oss.str());
		}

		if (name.size() > 1 && (name[0] == '/' || name[0] == '.')) {
			oss << "config error: domain name must not start with '/' or '.': "
				<< name << std::endl;
			throw std::runtime_error(oss.str());
		}

		if (name.size() > 1 && (name[name.size() - 1] == '/' || name[name.size() - 1] == '.')) {
			oss << "config error: domain name must not end with '/' or '.': "
				<< name << std::endl;
			throw std::runtime_error(oss.str());
		}

		// if (name.find("..") != std::string::npos) {
		// 	throw std::runtime_error("config error: domain name contains \"..\"");
		// }

		// Split by '.' and validate each label
		size_t dot_pos = 0;
		while (dot_pos < name.length()) {
			size_t next = name.find('.', dot_pos);
			if (next == std::string::npos) next = name.length();

			size_t label_len = next - dot_pos;
			if (label_len == 0) {
				throw std::runtime_error("config error: domain name contains \"..\"");
			} else if (label_len > 63) {
				oss << "config error: each dot-separated label must be under 64 chars long"
					<< "(RFC 1035)" << std::endl;
				throw std::runtime_error(oss.str());
			}

			// Check characters and hyphens
			for (size_t i = dot_pos; i < next; ++i) {
				if (!isalnum(static_cast<unsigned char>(name[i])) && name[i] != '-') {
					oss << "config error: domain name allowed characters: [a-zA-Z0-9.-]";
					throw std::runtime_error(oss.str());
				}
				if ((i == dot_pos || i == next - 1) && name[i] == '-') {
					oss << "config error: domain name: no leading/trailing hyphens"
						<< std::endl;
					throw std::runtime_error(oss.str());
				}
			}
			dot_pos = next + 1;
		}

		dom.names.push_back(name);

	}

	// size_t first_space = header.find(' ');
	// size_t last_space  = header.rfind(' ');
	// if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space) {
	// 	dom.name = trim(header.substr(first_space + 1, last_space - first_space - 1));
	// }
	// if (dom.name.empty()) {
	// 	throw std::runtime_error("config error: domain name directive requires a value");
	// }
	// if (dom.name.size() > 1 && dom.name[0] == '/') {
	// 	throw std::runtime_error("config error: domain name must not start with '/': " + dom.name);
	// }
	// if (dom.name.size() > 1 && dom.name[dom.name.size() - 1] == '/') {
	// 	throw std::runtime_error("config error: domain name must not end with '/': " + dom.name);
	// }
}

static void extractLocationPath(/*const std::string& root, */const std::string& header, Config::Location& loc) {

	size_t first_space = header.find(' ');
	size_t last_space  = header.rfind(' ');

	if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space) {
		loc.path = trim(header.substr(first_space + 1, last_space - first_space - 1));

	} else {
		loc.path = "/";
	}

	std::ostringstream oss;

	if (loc.path.empty()) {
		throw std::runtime_error("config error: location directive requires a value");
	}

	if (loc.path.size() > 1 && loc.path[0] != '/') {
		oss << "config error: location path must start with '/': "
			<< loc.path << std::endl;
		throw std::runtime_error(oss.str());
	}

	if (loc.path.size() > 1 && loc.path[loc.path.size() - 1] == '/') {
		oss << "config error: location path must not end with '/': "
			<< loc.path << std::endl;
		throw std::runtime_error(oss.str());
	}

	// if (!isDirectory(root + loc.path)) {
	// 	oss << "config error: location path does not exist or is not a directory: "
	// 	<< root + loc.path << std::endl;
	// 	throw std::runtime_error(oss.str());
	// }
	// if (mkdir((root + loc.path).c_str(), 0755) == -1 && errno != EEXIST) {
	// 	oss << "config error: no write access (location path): "
	// 		<< root + loc.path << std::endl;
	// 	throw std::runtime_error(oss.str());
	// }

	return;

}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

// unsigned long Parser::global_count = 0;

/*	@brief Instance	*/
ConfigLoader& ConfigLoader::instance(void) {
	static ConfigLoader instance;
	return instance;
}

// const std::vector<Config>& Parser::getAllConfigs() const {
// 	return _configs;
// }

// const Config& Parser::getConfig(size_t index) const {
// 	return _configs[index];
// }

// size_t Parser::getNumConfigs(void) const {
// 	return _configs.size();
// }

// const Location* Parser::matchLocation(const std::vector<Location>& locations,
// 									  const std::string& location_path) {
//
// 	// Look for exact match
// 	for (size_t i = 0; i < locations.size(); ++i) {
// 		if (location_path == locations[i].path) {
// 			return &locations[i];
// 		}
// 	}
//
// 	// Longest prefix match wins
// 	const Location*	matched_location = NULL;
// 	size_t matched_location_path_len = 0;
//
// 	for (size_t i = 0; i < locations.size(); ++i) {
//
// 		const Location* config_location = &locations[i];
// 		std::string config_location_path = config_location->path;
// 		size_t config_location_path_len = config_location_path.length();
// 		size_t location_path_len = location_path.length();
// 		// log.error(config_location_path + " " + i2a(config_location_path_len));
// 		// log.error(location_path + " " + i2a(location_path_len));
// 		if (startsWith(location_path,
// 			config_location_path,
// 			location_path_len,
// 			config_location_path_len)) {
//
// 			// log.error("A config location matches with requested location.");
// 			config_location_path_len =	config_location_path.length();
// 			bool is_valid_boundary =	(config_location_path_len == location_path_len ||
// 										location_path[config_location_path_len] == '/' ||
// 										config_location_path == "/");
// 			// log.error(std::string("") + (requested_location_path[config_location_path_len]));
// 			// log.error(is_valid_boundary ? "valid boundary" : "invalid boundary");
// 			// log.error(i2a(config_location_path_len) + " vs " + i2a(matched_location_path_len));
//
// 			if (is_valid_boundary && config_location_path_len > matched_location_path_len) {
// 				matched_location_path_len = config_location_path_len;
// 				matched_location = config_location;
// 				// log.error("New match found! " + matched_location->path + " " + i2a(config_location_path_len));
// 			}
// 		}
// 	}
//
// 	// if (matched_location != NULL)
// 	// 	log.error("HERE > " + matched_location->path + " < HERE");
// 	return matched_location;
//
// }

// Read line-by-line; fills config object
void ConfigLoader::config(const std::string& config_file) {

	// if (!validMethodsArrayValid()) {
	// 	throw std::runtime_error("valid methods enumeration mismatch");
	// }

	if(!isConfigFile(config_file)) {
		throw std::runtime_error("config error: invalid file extension: " + config_file);
	}

	std::ifstream file(config_file.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("permission denied: " + config_file);
	}

	// bool foundServer = false;
	bool found_endpoint = false;

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

		// if (trimmed == "server {") {
		// 	_parseServerBlock(file);
		// 	foundServer = true;

		if (trimmed == "socket {") {
			_parseSocketBlock(file);
			found_endpoint = true;

		} else if (trimmed == "socket") {
			throw std::runtime_error("config error: 'socket' directive requires '{'");

		} else {
			throw std::runtime_error("config error: unexpected directive outside socket block: " + trimmed);
		}

	}

	if (!found_endpoint) {
		throw std::runtime_error("config error: config file must contain at least one socket block");
	}

	file.close();

	// configs.validateRedirectChains();
	// Config::validateRedirectChains();
	_validateRedirectChains();

	return;

}

Method ConfigLoader::extractMethod(const std::string& method) {

	static const std::string valid_methods[
		static_cast<int>(METHOD_COUNT)
	] = {
		"GET", "HEAD", "DELETE", "POST", "PUT"
	};
	for (size_t i = 0; i < static_cast<int>(METHOD_COUNT); ++i) {
		// log.error(valid_methods[i]);
		if (valid_methods[i] == method) {
			// log.error("SET METHOD: " + valid_methods[i]);
			// _method = static_cast<Method>(i);
			// return;
			return static_cast<Method>(i);
		}
	}
	// log.error("OH NO! NO METHOD!! WHAT SHOULD WE DO??");
	return METHOD_COUNT;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
ConfigLoader::ConfigLoader(void) {
	log.debug("Parser Constructor called");
	return;
};

/*	@brief Copy Constructor	*/
ConfigLoader::ConfigLoader(const ConfigLoader& other)/* : _configs(other._configs) */{
	log.debug("Parser Copy Constructor called");
	*this = other;
	return;
};

/*	@brief Copy Assignment Operator	*/
ConfigLoader& ConfigLoader::operator=(const ConfigLoader& other) {
	if (this != &other) {
		log.debug("Parser Copy Assignment Operator called");
		// this->_configs = other._configs;
	}
	return *this;
};

/*	@brief Deconstructor	*/
ConfigLoader::~ConfigLoader() {
	log.debug("Parser Deconstructor called");
	return;
};

ConfigLoader::location_directive_handler_map ConfigLoader::_initLocationDirectiveHandlerMap(void) {

	location_directive_handler_map handlers;

	handlers["root"] = &ConfigLoader::_handleRoot;
	handlers["alias"] = &ConfigLoader::_handleAlias;
	handlers["redirect"] = &ConfigLoader::_handleRedirect;
	handlers["allow_methods"] = &ConfigLoader::_handleAllowedMethods;
	handlers["autoindex"] = &ConfigLoader::_handleAutoindex;
	handlers["index"] = &ConfigLoader::_handleIndexFile;
	handlers["upload_dir"] = &ConfigLoader::_handleUploadDirectory; // equivalent to nginx client_body_temp_path
	// handlers["cgi_ext"] = &Parser::_handleCGIExt;
	// handlers["cgi_path"] = &Parser::_handleCGIPath;
	handlers["interpreter"] = &ConfigLoader::_handleInterpreter;
	handlers["error_page"] = &ConfigLoader::_handleErrorPage;
	handlers["client_max_body_size"] = &ConfigLoader::_handleClientMaxBodySize;

	return handlers;

}

ConfigLoader::domain_directive_handler_map ConfigLoader::_initDomainDirectiveHandlerMap(void) {

	domain_directive_handler_map handlers;

	handlers["root"] = &ConfigLoader::_handleRoot;
	handlers["index"] = &ConfigLoader::_handleIndexFile;
	handlers["error_page"] = &ConfigLoader::_handleErrorPage;
	handlers["client_max_body_size"] = &ConfigLoader::_handleClientMaxBodySize;

	return handlers;
}

ConfigLoader::socket_directive_handler_map ConfigLoader::_initSocketDirectiveHandlerMap(void) {

	socket_directive_handler_map handlers;

	handlers["listen"] = &ConfigLoader::_handleListen;
	handlers["host"] = &ConfigLoader::_handleHost;
	handlers["client_max_body_size"] = &ConfigLoader::_handleClientMaxBodySize;

	return handlers;
}

void ConfigLoader::_parseLocationBlock(std::ifstream& config_file_stream,
								 Config::Domain& dom, Config::Location& loc) {

	static const location_directive_handler_map handlers = _initLocationDirectiveHandlerMap();

	// Falls back to domain treshold, if not specified for location
	loc.client_max_body_size = dom.client_max_body_size;

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
			// if (loc.cgi_extensions.size() != loc.cgi_paths.size()) {
			// 	throw std::runtime_error("config error: cgi_ext and cgi_path count mismatch in location '" + loc.path + "'");
			// }
			// Check for alias
			if (!loc.alias.empty() && !loc.root.empty()) {
				throw std::runtime_error("config error: \"alias\" and \"root\" directives are incompatible");
			}
			// Check for location root: if empty, substitute domain root
			if (loc.alias.empty() && loc.root.empty()) {
				loc.root = dom.root;
			}
			std::string path = loc.alias;
			if (!loc.root.empty()) path = loc.root;
			// else path = loc.alias;
			// Check for location index file(s): if empty, substitute domain index file(s)
			if (loc.index_files.empty()) {
				loc.index_files = dom.index_files;
			}
			// TODO
			// for (size_t i = 0; i < loc.index_files.size(); ++i) {
			// 	if (!isReadable(path + "/" + loc.index_files[i])) {
			// 		throw std::runtime_error("parse error: no read access: " + path + "/" + loc.index_files[i]);
			// 	}
			// }

			// Check for location error pages: if empty, substitute domain error pages
			if (loc.error_pages.empty()) {
				loc.error_pages = dom.error_pages;
			}
			std::map<int, std::string>::const_iterator err_it = loc.error_pages.begin();
			while (err_it != loc.error_pages.end()) {
				if (!isReadable(err_it->second)) {
					throw std::runtime_error("parse error: no read access: " + err_it->second);
				}
				++err_it;
			}

			// Check for upload directory if POST method allowed // TODO
			// for (size_t i = 0; i < loc.methods.size(); ++i) {
			// 	if (loc.methods[i] == POST && loc.upload_dir.empty()) {
			// 		std::ostringstream oss;
			// 		oss << "config error: location at path '" << loc.path
			// 			<< "' (under domain '" << dom.names[0]
			// 			<< "') allows POST, but upload directory is not set"
			// 			<< std::endl;
			// 		throw std::runtime_error(oss.str());
			// 	}
			// }

			// If client body treshold too high, set to global maximum
			if (loc.client_max_body_size > Config::SERVER_MAX_BODY_SIZE) {
				loc.client_max_body_size = Config::SERVER_MAX_BODY_SIZE;
			}

			// Check for duplicate paths
			if (isDuplicateLocation(dom.locations, loc.path)) {
				// std::ostringstream oss;
				// oss	<< "config error: location path " << loc.path
				// << " is already registered by another location block";
				// throw std::runtime_error(oss.str());
				throw std::runtime_error("config error: duplicate location path '" + loc.path + "'");
			}
			dom.locations.push_back(loc);
			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, loc);
		} else {
			throw std::runtime_error("config error: unknown directive in location block: " + key);
		}

	}

	// If we exit the loop without finding }, throw
	// If we get here, the location block was never closed
	throw std::runtime_error("config error: unclosed location block '" + loc.path + "' (missing '}')");

	return;

}

void ConfigLoader::_parseDomainBlock(std::ifstream& config_file_stream,
									 Config::Socket& soc, Config::Domain& dom) {

	static const domain_directive_handler_map handlers = _initDomainDirectiveHandlerMap();

	// Config::Domain dom_block;
	// server_block.port = 80;

	// Falls back to socket treshold, if not specified for location
	dom.client_max_body_size = soc.client_max_body_size;

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
			// Check for domain root
			if (dom.root.empty()) {
				throw std::runtime_error("config error: missing root directive");
			}
			const std::string path = dom.root + "/";
			// Check domain index file(s) // TODO
			// for (size_t i = 0; i < dom.index_files.size(); ++i) {
			// 	if (!isReadable(path + dom.index_files[i])) {
			// 		throw std::runtime_error("parse error: no read access: " + path + dom.index_files[i]);
			// 	}
			// }

			// Check domain error pages
			std::map<int, std::string>::const_iterator err_it = dom.error_pages.begin();
			while (err_it != dom.error_pages.end()) {
				if (!isReadable(err_it->second)) {
					throw std::runtime_error("parse error: no read access: " + err_it->second);
				}
				++err_it;
			}

			// If client body treshold too high, set to global maximum
			if (dom.client_max_body_size > Config::SERVER_MAX_BODY_SIZE) {
				dom.client_max_body_size = Config::SERVER_MAX_BODY_SIZE;
			}

			// Check for duplicate domain names
			for (size_t i = 0; i < dom.names.size(); ++i) {
				if (isDuplicateDomain(soc.domains, dom.names[i])) {
					// std::ostringstream oss;
					// oss	<< "config error: one of the following domain names: ";
					// for (size_t i = 0; i < dom.names.size(); ++i) {
					// 	oss << "'" << dom.names[i] << "'";
					// }
					// oss << " is already registered by another domain block";
					// oss	<< "config error: domain name " << dom.name
					// 	<< " is already registered by another domain block";
					// throw std::runtime_error(oss.str());
					throw std::runtime_error("config error: duplicate domain name '" + dom.names[i] + "'");
				}
			}
			soc.domains.push_back(dom);
			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, dom);
		} else if (key == "location") {
			Config::Location loc;
			loc.autoindex = false;
			// Read the location header line
			extractLocationPath(/*dom.root, */trimmed, loc);
			// Read the location block
			_parseLocationBlock(config_file_stream, dom, loc);
			continue;
		} else {
			throw std::runtime_error("config error: unknown directive in server block: " + key);
		}

	}

	// If we exit the loop without finding }, throw
	// If we get here, the domain block was never closed
	std::ostringstream oss;
	for (size_t i = 0; i < dom.names.size(); ++i) {
		oss << "'" << dom.names[i] << "'";
	}
	oss << std::endl;
	throw std::runtime_error("config error: unclosed domain block <" + oss.str() + "> (missing '}')");

	return;

}

void ConfigLoader::_parseSocketBlock(std::ifstream& config_file_stream) {

	static const socket_directive_handler_map handlers = _initSocketDirectiveHandlerMap();

	Config::Socket soc;
	// soc.port = 8080; // If no port set, set to 8080
	// soc.client_max_body_size = UINT64_MAX; // If client body treshold not set, set to 2^64

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
			// Check host address
			if (soc.address.empty()) {
				// soc.address = "0.0.0.0";
				throw std::runtime_error("config error: no host address set");
			}

			// If client body treshold too high, set to global maximum
			if (soc.client_max_body_size > Config::SERVER_MAX_BODY_SIZE) {
				soc.client_max_body_size = Config::SERVER_MAX_BODY_SIZE;
			}

			// Check for duplicate sockets
			if (isDuplicateSocket(configs.get(), soc.address, soc.port)) {
				// std::ostringstream oss;
				// oss	<< "config error: host " << soc.address
				// << " port " << soc.port
				// << " is already in use by another server block";
				// throw std::runtime_error(oss.str());
				throw std::runtime_error("config error: duplicate socket " + soc.address + ":" + i2a(soc.port));
			}

			configs.pushConfig(soc);
			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, soc);
		// if (key == "listen") {
		// 	_handleListen(val, soc);
		// } else if (key == "host") {
		// 	_handleHost(val, soc);
		// } else if (key == "client_max_body_size") {
		// 	_handleClientMaxBodySize(val, soc);
		} else if (key == "domain") {
			Config::Domain dom;
			// Read the domain header line
			extractDomainNames(trimmed, dom);
			// Read the domain block
			_parseDomainBlock(config_file_stream, soc, dom);
			continue;
		} else {
			throw std::runtime_error("config error: unknown directive in socket block: " + key);
		}

	}

	// If we exit the loop without finding }, throw
	// If we get here, the socket block was never closed
	throw std::runtime_error("config error: unclosed socket block " + soc.address + ":" + i2a(soc.port) + " (missing '}')");
	return;

}

// void Parser::_handleRoot(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: root directive requires a value");
// 	}

// 	if (val[0] != '/') {
// 		throw std::runtime_error("config error: root path must start with '/': " + val);
// 	}

// 	if (val[val.size() - 1] == '/') {
// 		throw std::runtime_error("config error: root path must not end with '/': " + val);
// 	}

// 	loc.root = val;

// 	return;

// }

void ConfigLoader::_handleAlias(const std::string& val, Config::Location& loc) {

	// if (!loc.root.empty()) {
	// 	throw std::runtime_error("config error: \"alias\" and \"root\" directives are incompatible");
	// }

	if (val.empty()) {
		throw std::runtime_error("config error: alias directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("config error: alias value must start with '/': " + val);
	}

	if (val[val.size() - 1] == '/') {
		throw std::runtime_error("config error: alias value must not end with '/': " + val);
	}

	if (mkdir(val.c_str(), 0755) == -1 && errno != EEXIST) {
		throw std::runtime_error("config error: no write access (alias directory): " + val);
	}

	loc.alias = val;

	return;

}

void ConfigLoader::_handleRedirect(const std::string& val, Config::Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("config error: redirect directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("config error: redirect value must start with '/': " + val);
	}

	if (val[val.size() - 1] != '/') {
		throw std::runtime_error("config error: redirect value must end with '/': " + val);
	}

	loc.redirect = val;

	return;

}

void ConfigLoader::_handleAllowedMethods(const std::string& val, Config::Location& loc) {

	// if (!loc.methods.empty())
		// log.debug(loc.methods.back());

	std::istringstream iss(val);
	std::string token;

	while (iss >> token) {

		// if (!isSupportedMethod(token)) {
		const Method method = extractMethod(token);
		if (method >= METHOD_COUNT) {
			throw std::runtime_error("config error: invalid HTTP method: " + token);
		}
		loc.methods.push_back(method);
	}

	return;

}

void ConfigLoader::_handleAutoindex(const std::string& val, Config::Location& loc) {

	if (val != "on" && val != "off") {
		throw std::runtime_error("config error: autoindex must be 'on' or 'off', got: " + val);
	}

	loc.autoindex = (val == "on");

	return;

}

// void Parser::_handleIndexFile(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: index_files directive requires at least one file");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		// if (val.empty()) {
// 		// 	throw std::runtime_error("config error: index file directive requires a value");
// 		// } // redundant, see "if (!(iss >> path))"

// 		if (val[0] == '/') {
// 			throw std::runtime_error("config error: index file path must not start with '/': " + val);
// 		}

// 		if (val[val.size() - 1] == '/') {
// 			throw std::runtime_error("config error: index file path must not end with '/': " + val);
// 		}

// 		if (!isReadable(loc.root + "/" + path)) {
// 			throw std::runtime_error("config error: no read access: " + path);
// 		}

// 		loc.index_files.push_back(path);

// 	}

// 	return;

// }

void ConfigLoader::_handleUploadDirectory(const std::string& val, Config::Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("config error: upload_dir directive requires a value");
	}

	if (val[0] != '/') {
		throw std::runtime_error("config error: upload directory path must start with '/': " + val);
	}

	if (val[val.size() - 1] == '/') {
		throw std::runtime_error("config error: upload directory path must not end with '/': " + val);
	}

	// if (!isWritable(val)) {
	// 	throw std::runtime_error("config error: no write access (upload directory): " + val);
	// }
	std::string path = loc.alias;
	if (!loc.root.empty()) path = loc.root + loc.path;
	if (((mkdir((path + val).c_str(), 0755) != 0) && (errno != EEXIST))/* ||
		((mkdir((path + val + "/temp").c_str(), 0755) != 0) && (errno != EEXIST))*/) {
		throw std::runtime_error("config error: no write access (upload directory): " + val);
	}

	loc.upload_dir = val;

	return;

}

// void Parser::_handleCGIExt(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: cgi_ext directive requires at least one extension");
// 	}

// 	std::istringstream iss(val);
// 	std::string ext;

// 	while (iss >> ext) {

// 		if (!isSupportedCGIExtension(ext)) {
// 			throw std::runtime_error("config error: invalid CGI extension: " + ext);
// 		}

// 		loc.cgi_extensions.push_back(ext);

// 	}

// 	return;

// }

// void Parser::_handleCGIPath(const std::string& val, Location& loc) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: cgi_path directive requires at least one path");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		if (path[0] != '/') {
// 			throw std::runtime_error("config error: CGI path must start with '/': " + path);
// 		}

// 		if (path[path.size() - 1] == '/') {
// 			throw std::runtime_error("config error: CGI path must not end with '/': " + path);
// 		}

// 		if (!isExecutable(path)) {
// 			throw std::runtime_error("config error: invalid CGI path (not executable): " + path);
// 		}

// 		loc.cgi_paths.push_back(path);

// 	}

// 	return;

// }

void ConfigLoader::_handleInterpreter(const std::string& val, Config::Location& loc) {

	if (val.empty()) {
		throw std::runtime_error("config error: interpreter directive requires extension and path");
	}

	std::istringstream iss(val);
	std::string ext;
	std::string path;

	if (!(iss >> ext >> path)) {
		throw std::runtime_error("config error: interpreter requires both extension and path");
	}

	if (ext.empty() || ext[0] != '.' || ext.find('/') != std::string::npos) {
		throw std::runtime_error("config error: invalid CGI extension: " + ext);
	}

	// if (path.empty()) {
	// 	throw std::runtime_error("config error: interpreter directive requires a value");
	// } // redundant, see "if (!(iss >> ext >> path))"

	if (path[0] != '/') {
		throw std::runtime_error("config error: interpreter path must start with '/': " + path);
	}

	if (path[path.size() - 1] == '/') {
		throw std::runtime_error("config error: interpreter path must not end with '/': " + path);
	}

	if (!isExecutable(path)) {
		throw std::runtime_error("config error: interpreter path not executable: " + path);
	}

	if (loc.interpreters.count(ext) > 0) {
		throw std::runtime_error("config error: CGI extension '" + ext
									+ "' already mapped in location '" + loc.path + "'");
	}

	loc.interpreters[ext] = path;

	return;

}

// void Parser::_handleErrorPage(const std::string& val, Location& loc) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("config error: error_page directive requires code and path");
// 	}
//
// 	std::istringstream iss(val);
// 	std::string code_str;
// 	std::string path;
//
// 	if (!(iss >> code_str >> path)) {
// 		throw std::runtime_error("config error: error_page directive requires both code and path");
// 	}
//
// 	int code = stringToInt(code_str);
//
// 	if (!isValidErrorCode(code)) {
// 		throw std::runtime_error("config error: invalid error code: " + code_str);
// 	}
//
// 	// if (path.empty()) {
// 	// 	throw std::runtime_error("config error: error page directive requires a value");
// 	// } // redundant, see "if (!(iss >> code_str >> path))"
//
// 	if (path[0] == '/') {
// 		throw std::runtime_error("config error: error page file path must not start with '/': " + path);
// 	}
//
// 	if (path[path.size() - 1] == '/') {
// 		throw std::runtime_error("config error: error page file path must not end with '/': " + path);
// 	}
//
// 	if (!isReadable(path)) {
// 		throw std::runtime_error("config error: no read access: " + path);
// 	}
//
// 	loc.error_pages[code] = path;
//
// 	return;
//
// }

void ConfigLoader::_handleListen(const std::string& val, Config::Socket& config) {

	if (!isValidPort(val)) {
		throw std::runtime_error("config error: invalid port value: " + val);
	}

	config.port = stringToUnsignedShort(val);

	return;

}

void ConfigLoader::_handleHost(const std::string& val, Config::Socket& config) {

	if (!isValidIPAddress(val)) {
		throw std::runtime_error("config error: invalid host IP: " + val);
	}

	config.address = val;

	return;

}

// void Parser::_handleServerNames(const std::string& val, Config& config) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("config error: server_name directive requires at least one name");
// 	}
//
// 	std::istringstream iss(val);
// 	std::string name;
//
// 	while (iss >> name) {
//
// 		if (name.empty()) {
// 			throw std::runtime_error("config error: empty server_name");
// 		}
//
// 		config.server_names.push_back(name);
//
// 	}
//
// 	return;
//
// }

// void Parser::_handleRoot(const std::string& val, Config& config) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: root directive requires a value");
// 	}

// 	if (val[0] != '/') {
// 		throw std::runtime_error("config error: root path must start with '/': " + val);
// 	}

// 	if (val[val.size() - 1] == '/') {
// 		throw std::runtime_error("config error: root path must not end with '/': " + val);
// 	}

// 	config.root = val;

// 	return;

// }

// void Parser::_handleIndexFile(const std::string& val, Config& config) {

// 	if (val.empty()) {
// 		throw std::runtime_error("config error: index_files directive requires at least one file");
// 	}

// 	std::istringstream iss(val);
// 	std::string path;

// 	while (iss >> path) {

// 		// if (val.empty()) {
// 		// 	throw std::runtime_error("config error: index file directive requires a value");
// 		// } // redundant, see "if (!(iss >> path))"

// 		if (val[0] == '/') {
// 			throw std::runtime_error("config error: index file path must not start with '/': " + val);
// 		}

// 		if (val[val.size() - 1] == '/') {
// 			throw std::runtime_error("config error: index file path must not end with '/': " + val);
// 		}

// 		if (!isReadable(config.root + "/" + path)) {
// 			throw std::runtime_error("config error: no read access: " + path);
// 		}

// 		config.index_files.push_back(path);

// 	}

// 	return;

// }

// void Parser::_handleErrorPage(const std::string& val, Config& config) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("config error: error_page directive requires code and path");
// 	}
//
// 	std::istringstream iss(val);
// 	std::string code_str;
// 	std::string path;
//
// 	if (!(iss >> code_str >> path)) {
// 		throw std::runtime_error("config error: error_page requires both status code and path");
// 	}
//
// 	int code = stringToInt(code_str);
//
// 	if (!isValidErrorCode(code)) {
// 		throw std::runtime_error("config error: invalid error code: " + code_str);
// 	}
//
// 	// if (path.empty()) {
// 	// 	throw std::runtime_error("config error: error page directive requires a value");
// 	// } // redundant, see "if (!(iss >> code_str >> path))"
//
// 	if (path[0] == '/') {
// 		throw std::runtime_error("config error: error page file path must not start with '/': " + path);
// 	}
//
// 	if (path[path.size() - 1] == '/') {
// 		throw std::runtime_error("config error: error page file path must not end with '/': " + path);
// 	}
//
// 	if (!isReadable(path)) {
// 		throw std::runtime_error("config error: no read access: " + path);
// 	}
//
// 	config.error_pages[code] = path;
//
// 	return;
//
// }

// void Parser::_handleClientMaxBodySize(const std::string& val, Config::Domain& dom) {
//
// 	if (val.empty()) {
// 		throw std::runtime_error("config error: client_max_body_size requires a value");
// 	}
//
// 	size_t size = stringToSize(val);
//
// 	if (!isValidBodySize(size)) {
// 		throw std::runtime_error("config error: invalid client_max_body_size: " + val);
// 	}
//
// 	if (size == 0) dom.client_max_body_size = ULONG_MAX;
// 	else dom.client_max_body_size = size;
//
// 	return;
//
// }

void ConfigLoader::_validateRedirectChains(void) {

	std::ostringstream oss;
	std::vector<Config::Socket>::const_iterator conf_it = configs.get().begin();
	while (conf_it != configs.get().end()) {
		std::vector<Config::Domain>::const_iterator dom_it = conf_it->domains.begin();
		while (dom_it != conf_it->domains.end()) {
			std::vector<Config::Location>::const_iterator loc_it = dom_it->locations.begin();
			while (loc_it != dom_it->locations.end()) {
				unsigned short redirection_count = 0;
				std::vector<std::string> redirects;
				const Config::Location* loc = &(*loc_it);
				std::string redirect = loc->path;
				if (redirect != "/") redirect.append("/");
				redirects.push_back(redirect);
				while (!loc->redirect.empty()) {
					if (redirect == loc->redirect) {
						throw std::runtime_error("config error: self-redirect at '" + loc->path + "'");
					}
					// if (redirects.count(loc->redirect)) {
					if (std::find(redirects.begin(), redirects.end(), loc->redirect) != redirects.end()) {
						// log.error("config error: circular redirect detected at '" + loc->redirect + "'");
						// log.error("redirect chain: ");
						// // for (std::vector<std::string>::iterator it = redirects.begin(); it != redirects.end(); ++it) {
						// // std::cerr << "\e[31m" << *it << " -> \e[0m";
						// // log.error(*it);
						// for (size_t i = 0; i < redirects.size(); ++i) {
						// 	log.error(redirects[i]);
						// }
						// throw std::runtime_error(loc->redirect + " (LOOP)");
						oss << "config error: circular redirect detected at '" << loc->redirect << "'\nredirect chain:\n";
						for (size_t i = 0; i < redirects.size(); ++i) {
							oss << redirects[i] << "\n";
						}
						oss << loc->redirect << " (LOOP)" << std::endl;
						throw std::runtime_error(oss.str());
					}
					++redirection_count;
					if (redirection_count > MAX_REDIRECTS) {
						// log.error("config error: too many consecutive redirects");
						// log.error("redirect chain: ");
						// // for (std::vector<std::string>::iterator it = redirects.begin(); it != redirects.end(); ++it) {
						// // std::cerr << "\e[31m" << *it << " -> \e[0m";
						// // log.error(*it);
						// for (size_t i = 0; i < redirects.size(); ++i) {
						// 	log.error(redirects[i]);
						// }
						// log.error(loc->redirect);
						// throw std::runtime_error(i2a(redirects.size()) + "/" + i2a(MAX_REDIRECTS) + " hops");
						oss << "config error: too many consecutive redirects\nredirect chain:\n";
						for (size_t i = 0; i < redirects.size(); ++i) {
							oss << redirects[i] << "\n";
						}
						oss << loc->redirect << "\n";
						oss << i2a(redirects.size()) << "/" << i2a(MAX_REDIRECTS) << " hops" << std::endl;
						throw std::runtime_error(oss.str());
					}
					redirects.push_back(loc->redirect);
					loc = const_cast<Config::Location*>(Dispatcher::resolveLocation(dom_it->locations, loc->redirect));
					// if (!Dispatcher::matchLocation(dom_it->locations, loc->redirect, *loc)) {
					if (loc == NULL) {
						log.error("config error: no matching location");
						return;
					}
				}
				++loc_it;
			}
			++dom_it;
		}
		++conf_it;
	}
	// return true;
	return;
}
