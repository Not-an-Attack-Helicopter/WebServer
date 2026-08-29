/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLoader.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:41:59 by sholz             #+#    #+#             */
/*   Updated: 2026/08/24 21:49:19 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

// #include "HTTPResponse.hpp"
// #include "Config.hpp"
#include "utils.hpp"
// #include <stdexcept>
// #include <sstream>
// #include <vector>
// #include <string>
// #include <map>
// #include <climits>
// #include <cstddef>
// #include <typeinfo>
#include <sys/stat.h>

#define load ConfigLoader::instance()

class ConfigLoader {

public:

	static ConfigLoader&						instance(void);

	void										config(const std::string& config_file);

	Method										extractMethod(const std::string& name);

	typedef void (ConfigLoader::*locationDirectiveHandler) (const std::string& val,
													  Config::Location& loc);

	typedef void (ConfigLoader::*domainDirectiveHandler) (const std::string& val,
													Config::Domain& domain);

	typedef void (ConfigLoader::*socketDirectiveHandler) (const std::string& val,
													Config::Socket& socket);

	typedef std::map<std::string,
					locationDirectiveHandler> location_directive_handler_map;

	typedef std::map<std::string,
					domainDirectiveHandler> domain_directive_handler_map;

	typedef std::map<std::string,
					socketDirectiveHandler> socket_directive_handler_map;

private:

	ConfigLoader(void);
	ConfigLoader(const ConfigLoader& other);
	ConfigLoader& operator = (const ConfigLoader& other);
	~ConfigLoader(void);

	static const unsigned short				MAX_REDIRECTS = 5;

	template<typename ConfigType>
	void _handleRoot(const std::string& val, ConfigType& block) {

		if (val.empty()) {
			throw std::runtime_error("parse error: root directive requires a value");
		}

		if (val[0] != '/') {
			throw std::runtime_error("parse error: root path must start with '/': " + val);
		}

		if (val[val.size() - 1] == '/') {
			throw std::runtime_error("parse error: root path must not end with '/': " + val);
		}

		if (mkdir(val.c_str(), 0755) == -1 && errno != EEXIST) {
			throw std::runtime_error("config error: no write access (root directory): " + val);
		}

		block.root = val;
		return;

	}

	template<typename ConfigType>
	void _handleIndexFile(const std::string& val, ConfigType& block) {

		if (val.empty()) {
			throw std::runtime_error("parse error: index_files directive requires at least one file");
		}

		std::istringstream iss(val);
		std::string path;

		while (iss >> path) {

			if (val[0] == '/') {
				throw std::runtime_error("parse error: index file path must not start with '/': " + val);
			}

			if (val[val.size() - 1] == '/') {
				throw std::runtime_error("parse error: index file path must not end with '/': " + val);
			}

			block.index_files.push_back(path);

		}

		return;

	}

	template<typename ConfigType>
	void _handleErrorPage(const std::string& val, ConfigType& block) {

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

		if (code < static_cast<int>(BAD_REQUEST) ||
			static_cast<int>(NETWORK_AUTHENTICATION_REQUIRED) < code) {
			throw std::runtime_error("parse error: invalid error code: " + code_str);
		}

		if (path[0] != '/') {
			throw std::runtime_error("parse error: error page file path must start with '/': " + path);
		}

		if (path[path.size() - 1] == '/') {
			throw std::runtime_error("parse error: error page file path must not end with '/': " + path);
		}

		block.error_pages[code] = path;

		return;

	}

	template<typename ConfigType>
	void _handleClientMaxBodySize(const std::string& val, ConfigType& block) {

		if (val.empty()) {
			throw std::runtime_error("config error: client_max_body_size requires a value");
		}

		std::size_t size = stringToSize(val);

		if (size != 0) block.client_max_body_size = size;

		return;

	}

	static location_directive_handler_map	_initLocationDirectiveHandlerMap(void);

	static domain_directive_handler_map		_initDomainDirectiveHandlerMap(void);

	static socket_directive_handler_map		_initSocketDirectiveHandlerMap(void);

	void									_parseLocationBlock(std::ifstream& config_file_stream,
																Config::Domain& dom,
																Config::Location& loc);

	void									_parseDomainBlock(std::ifstream& config_file_stream,
															  Config::Socket& soc,
															  Config::Domain& dom);

	void									_parseSocketBlock(std::ifstream& config_file_stream);

	// Domain & Location directive handlers
	void									_handleAlias(const std::string& val, Config::Location& loc);
	void									_handleRedirect(const std::string& val, Config::Location& loc);
	void									_handleAllowedMethods(const std::string& val, Config::Location& loc);
	void									_handleAutoindex(const std::string& val, Config::Location& loc);
	void									_handleUploadDirectory(const std::string& val, Config::Location& loc);
	void									_handleInterpreter(const std::string& val, Config::Location& loc);

	// Endpoint directive handlers
	void									_handleListen(const std::string& val, Config::Socket& soc);
	void									_handleHost(const std::string& val, Config::Socket& soc);

	void									_validateRedirectChains(void);

};

#endif
