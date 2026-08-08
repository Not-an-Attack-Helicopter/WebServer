/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:41:59 by sholz             #+#    #+#             */
/*   Updated: 2026/07/04 18:15:14 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

// #include "Config.hpp"
// #include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "utils.hpp"
#include <sys/stat.h>
#include <stdexcept>
// #include <typeinfo>
#include <sstream>
// #include <vector>
#include <string>
#include <map>
#include <cstddef>

#define parse Parser::instance()

class Parser {

public:

	static Parser&							instance(void);

	// const std::vector<Config>&				getAllConfigs(void) const;

	// const Config& 							getConfig(size_t index) const;

	// size_t									getNumConfigs(void) const;

	// static const Location*					matchLocation(const std::vector<Location>& locations,
	// 													  const std::string& location_path);

	void									configFile(const std::string& config_file);
	HTTPRequest::ParseState					incomingData(const std::string& raw, HTTPRequest* request);

	typedef void (Parser::*locationDirectiveHandler) (const std::string& val,
													  Config::Location& loc);

	typedef void (Parser::*domainDirectiveHandler) (const std::string& val,
													Config::Domain& domain);

	typedef std::map<std::string,
					locationDirectiveHandler> location_directive_handler_map;

	typedef std::map<std::string,
					domainDirectiveHandler> domain_directive_handler_map;

	Method									extractMethod(const std::string& name);

private:

	Parser(void);
	Parser(const Parser& other);
	Parser& operator = (const Parser& other);
	~Parser(void);

	static const unsigned short				MAX_REDIRECTS = 5;

	static const size_t 					LF_SIZE = 1;
	static const size_t 					CRLF_SIZE = 2;
	static const size_t						LF_LF_SIZE = 2;
	static const size_t						CRLF_CRLF_SIZE = 4;

	// Config file parsing //
	template<typename ConfigType>
	void _handleRoot(const std::string& val, ConfigType& block) {

		// if (typeid(block) == typeid(Config::Location)) {
		// 	if (!block.alias.empty()) {
		// 		throw std::runtime_error("config error: \"alias\" and \"root\" directives are incompatible");
		// 	}
		// }

		if (val.empty()) {
			throw std::runtime_error("parse error: root directive requires a value");
		}

		if (val[0] != '/') {
			throw std::runtime_error("parse error: root path must start with '/': " + val);
		}

		if (val[val.size() - 1] == '/') {
			throw std::runtime_error("parse error: root path must not end with '/': " + val);
		}

		// if (!isDirectory(val)) {
		// 	std::ostringstream oss;
		// 	oss << "config error: location path does not exist or is not a directory: "
		// 		<< val << std::endl;
		// 	throw std::runtime_error(oss.str());
		// }
		if (mkdir(val.c_str(), 0755) == -1 && errno != EEXIST) {
			throw std::runtime_error("config error: no write access (root directory): " + val);
		}

		block.root = val;

	}

	template<typename ConfigType>
	void _handleIndexFile(const std::string& val, ConfigType& block) {

		if (val.empty()) {
			throw std::runtime_error("parse error: index_files directive requires at least one file");
		}

		std::istringstream iss(val);
		std::string path;

		while (iss >> path) {

			// if (val.empty()) {
			// 	throw std::runtime_error("parse error: index file directive requires a value");
			// } // redundant, see "if (val.empty())"

			if (val[0] == '/') {
				throw std::runtime_error("parse error: index file path must not start with '/': " + val);
			}

			if (val[val.size() - 1] == '/') {
				throw std::runtime_error("parse error: index file path must not end with '/': " + val);
			}

			// if (!isReadable(block.root + "/" + path)) {
			// 	throw std::runtime_error("parse error: no read access: " + path);
			// }

			block.index_files.push_back(path);

		}

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

		// if (!isValidErrorCode(code)) {
		if (code < BAD_REQUEST ||
			NETWORK_AUTHENTICATION_REQUIRED < code) {
			throw std::runtime_error("parse error: invalid error code: " + code_str);
		}

		// if (path.empty()) {
		// 	throw std::runtime_error("parse error: error page directive requires a value");
		// } // redundant, see "if (!(iss >> code_str >> path))"

		if (path[0] != '/') {
			throw std::runtime_error("parse error: error page file path must start with '/': " + path);
		}

		if (path[path.size() - 1] == '/') {
			throw std::runtime_error("parse error: error page file path must not end with '/': " + path);
		}

		// if (!isReadable(path)) {
		// 	throw std::runtime_error("parse error: no read access: " + path);
		// }

		block.error_pages[code] = path;

		return;

	}

	static location_directive_handler_map	_initLocationDirectiveHandlerMap(void);

	static domain_directive_handler_map		_initDomainDirectiveHandlerMap(void);

	void									_parseLocationBlock(std::ifstream& config_file_stream,
																Config::Domain& dom,
																Config::Location& loc);

	void									_parseDomainBlock(std::ifstream& config_file_stream,
															  Config::Socket& soc,
															  Config::Domain& dom);

	void									_parseSocketBlock(std::ifstream& config_file_stream);

	// Location directive handlers
	// void									_handleRoot(const std::string& val, Config::Location& loc);
	void									_handleAlias(const std::string& val, Config::Location& loc);
	void									_handleRedirect(const std::string& val, Config::Location& loc);
	void									_handleAllowedMethods(const std::string& val, Config::Location& loc);
	void									_handleAutoindex(const std::string& val, Config::Location& loc);
	// void									_handleIndexFile(const std::string& val, Location& loc);
	void									_handleUploadDirectory(const std::string& val, Config::Location& loc);
	// void									_handleCGIExt(const std::string& val, Location& loc);
	// void									_handleCGIPath(const std::string& val, Location& loc);
	void									_handleInterpreter(const std::string& val, Config::Location& loc);
	// void									_handleErrorPage(const std::string& val, Location& loc);

	// Domain directive handlers
	// void									_handleDomainName(const std::string& val, Config::Domain& dom);
	// void									_handleRoot(const std::string& val, Config::Domain& dom);
	// void									_handleIndexFile(const std::string& val, Config::Domain& dom);
	// void									_handleErrorPage(const std::string& val, Config::Domain& dom);
	void									_handleClientMaxBodySize(const std::string& val, Config::Domain& dom);

	// Endpoint directive handlers
	void									_handleListen(const std::string& val, Config::Socket& soc);
	void									_handleHost(const std::string& val, Config::Socket& soc);

	void									_validateRedirectChains(void);

	// HTTP request parsing //
	size_t									_findRequestLineEnd(const std::string& raw, HTTPRequest& request);

	// bool									_matchMethod(const std::string& method);
	bool									_extractTokens(const std::string& line, HTTPRequest& request);
	bool									_parseHeaderLine(const std::string& line, HTTPRequest& request);
	// bool									_extractContentLength(void);

	HTTPRequest::ParseState					_parseRequestLine(const std::string& raw, HTTPRequest& request);
	HTTPRequest::ParseState					_parseHeaders(const std::string& raw, HTTPRequest& request);
	HTTPRequest::ParseState					_parseBody(const std::string& raw, HTTPRequest& request);

	// std::vector<Config>					_configs;

};

// #include "Parser.tpp"

#endif
