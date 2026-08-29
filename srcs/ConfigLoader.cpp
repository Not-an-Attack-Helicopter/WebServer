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
// #include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
// #include "../incs/utils.hpp"
// #include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
// #include <cstddef>
// #include <cctype>
// #include <vector>
// #include <fstream>
// #include <iostream>
#include <algorithm>
// #include <stdexcept>

// Config file parsing helpers
static bool isConfigFile(const std::string& filename) {

	std::size_t dot = filename.rfind('.');

	if (dot == std::string::npos || dot == filename.size() - 1) {
		return false;
	}

	std::string ext = filename.substr(dot + 1);

	return (ext == "conf");
}

static std::string stripInlineComment(std::string line) {

	std::size_t commentPos = line.find('#');
	if (commentPos != std::string::npos) {
		line = line.substr(0, commentPos);
	}

	return line;

}

static bool isSupportedCGIExtension(const std::string& ext) {

	const std::string valid_exts[] = {".py", ".sh"};
	const std::size_t size = arraySize(valid_exts);

	return (std::find(valid_exts, valid_exts + size, ext) != valid_exts + size);

}

static bool isReadable(const std::string& path) {

	if (!isRegularFile(path)) return false;

	int fd = open(path.c_str(), O_RDONLY);
	if (fd == -1) return false;
	close(fd);
	return true;

}

static bool isExecutable(const std::string& path) {

	if (!isRegularFile(path)) return false;

	return access(path.c_str(), X_OK) == 0;

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

		for (std::size_t i = 0; i < token.size(); ++i) {

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

static bool isDuplicateSocket(const std::vector<Config::Socket>& sockets,
							  const std::string& address, int port) {

	for (std::size_t i = 0; i < sockets.size(); ++i) {
		if (sockets[i].port == port && sockets[i].address == address) {
			return true;
		}
	}
	return false;

}

static bool isDuplicateDomain(const std::vector<Config::Domain>& domains,
							  std::string& name) {

	for (std::size_t i = 0; i < domains.size(); ++i) {
		for (std::size_t j = 0; j < domains[i].names.size(); ++j) {
			if (domains[i].names[j] == name) {
				return true;
			}
		}
	}
	return false;
}

static bool isDuplicateLocation(const std::vector<Config::Location>& locations,
								const std::string& path) {

	for (std::size_t i = 0; i < locations.size(); ++i) {
		if (locations[i].path == path) {
			return true;
		}
	}
	return false;
}

static std::string extractDirectiveKey(const std::string& line) {

	std::size_t space = line.find(' ');
	if (space == std::string::npos) {
		return line;
	}
	return line.substr(0, space);

}

static std::string extractDirectiveValue(const std::string& line) {

	std::size_t space = line.find(' ');
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

	std::size_t first_pos = header.find(' ');
	std::size_t last_pos  = header.rfind(' ');
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

		// Split by '.' and validate each label
		std::size_t dot_pos = 0;
		while (dot_pos < name.length()) {
			std::size_t next = name.find('.', dot_pos);
			if (next == std::string::npos) next = name.length();

			std::size_t label_len = next - dot_pos;
			if (label_len == 0) {
				throw std::runtime_error("config error: domain name contains \"..\"");
			} else if (label_len > 63) {
				oss << "config error: each dot-separated label must be under 64 chars long"
					<< "(RFC 1035)" << std::endl;
				throw std::runtime_error(oss.str());
			}

			// Check characters and hyphens
			for (std::size_t i = dot_pos; i < next; ++i) {
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

}

static void extractLocationPath(const std::string& header, Config::Location& loc) {

	std::size_t first_space = header.find(' ');
	std::size_t last_space  = header.rfind(' ');

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

	return;

}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
ConfigLoader& ConfigLoader::instance(void) {
	static ConfigLoader instance;
	return instance;
}

// Read line-by-line; fills config object
void ConfigLoader::config(const std::string& config_file) {

	if(!isConfigFile(config_file)) {
		throw std::runtime_error("config error: invalid file extension: " + config_file);
	}

	std::ifstream file(config_file.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("permission denied: " + config_file);
	}

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

	_validateRedirectChains();

	return;

}

Method ConfigLoader::extractMethod(const std::string& method) {

	static const std::string valid_methods[
		static_cast<int>(METHOD_COUNT)
	] = {
		"GET", "HEAD", "DELETE", "POST", "PUT"
	};
	for (std::size_t i = 0; i < static_cast<int>(METHOD_COUNT); ++i) {
		if (valid_methods[i] == method) {
			return static_cast<Method>(i);
		}
	}
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
ConfigLoader::ConfigLoader(const ConfigLoader& other) {
	log.debug("Parser Copy Constructor called");
	*this = other;
	return;
};

/*	@brief Copy Assignment Operator	*/
ConfigLoader& ConfigLoader::operator=(const ConfigLoader& other) {
	if (this != &other) {
		log.debug("Parser Copy Assignment Operator called");
	}
	return *this;
};

/*	@brief Destructor	*/
ConfigLoader::~ConfigLoader() {
	log.debug("Parser Destructor called");
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

			// Check for location index file(s): if empty, substitute domain index file(s)
			if (loc.index_files.empty()) {
				loc.index_files = dom.index_files;
			}

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

			// If client body treshold too high, set to global maximum
			if (loc.client_max_body_size > Config::SERVER_MAX_BODY_SIZE) {
				loc.client_max_body_size = Config::SERVER_MAX_BODY_SIZE;
			}

			// Check for duplicate paths
			if (isDuplicateLocation(dom.locations, loc.path)) {
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
			for (std::size_t i = 0; i < dom.names.size(); ++i) {
				if (isDuplicateDomain(soc.domains, dom.names[i])) {
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
			extractLocationPath(trimmed, loc);
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
	for (std::size_t i = 0; i < dom.names.size(); ++i) {
		oss << "'" << dom.names[i] << "'";
	}
	oss << std::endl;
	throw std::runtime_error("config error: unclosed domain block <" + oss.str() + "> (missing '}')");

	return;

}

void ConfigLoader::_parseSocketBlock(std::ifstream& config_file_stream) {

	static const socket_directive_handler_map handlers = _initSocketDirectiveHandlerMap();

	Config::Socket soc;

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
				throw std::runtime_error("config error: no host address set");
			}

			// If client body treshold too high, set to global maximum
			if (soc.client_max_body_size > Config::SERVER_MAX_BODY_SIZE) {
				soc.client_max_body_size = Config::SERVER_MAX_BODY_SIZE;
			}

			// Check for duplicate sockets
			if (isDuplicateSocket(configs.get(), soc.address, soc.port)) {
				throw std::runtime_error("config error: duplicate socket " + soc.address + ":" + i2a(soc.port));
			}

			configs.pushConfig(soc);
			return;

		}

		std::string key = extractDirectiveKey(trimmed);
		std::string val = extractDirectiveValue(trimmed);

		if (handlers.find(key) != handlers.end()) {
			(this->*handlers.at(key))(val, soc);
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

void ConfigLoader::_handleAlias(const std::string& val, Config::Location& loc) {

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

	std::istringstream iss(val);
	std::string token;

	while (iss >> token) {

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

	std::string path = loc.alias;
	if (!loc.root.empty()) path = loc.root + loc.path;
	if (((mkdir((path + val).c_str(), 0755) != 0) && (errno != EEXIST))) {
		throw std::runtime_error("config error: no write access (upload directory): " + val);
	}

	loc.upload_dir = val;

	return;

}

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

	if (!isSupportedCGIExtension(ext)) {
		throw std::runtime_error("config error: unsupported CGI extension: " + ext);
	}

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
					if (std::find(redirects.begin(), redirects.end(), loc->redirect) != redirects.end()) {
						oss << "config error: circular redirect detected at '" << loc->redirect << "'\nredirect chain:\n";
						for (std::size_t i = 0; i < redirects.size(); ++i) {
							oss << redirects[i] << "\n";
						}
						oss << loc->redirect << " (LOOP)" << std::endl;
						throw std::runtime_error(oss.str());
					}
					++redirection_count;
					if (redirection_count > MAX_REDIRECTS) {
						oss << "config error: too many consecutive redirects\nredirect chain:\n";
						for (std::size_t i = 0; i < redirects.size(); ++i) {
							oss << redirects[i] << "\n";
						}
						oss << loc->redirect << "\n";
						oss << i2a(redirects.size()) << "/" << i2a(MAX_REDIRECTS) << " hops" << std::endl;
						throw std::runtime_error(oss.str());
					}
					redirects.push_back(loc->redirect);
					loc = const_cast<Config::Location*>(Dispatcher::resolveLocation(dom_it->locations, loc->redirect));
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
	return;
}
