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

#include "Config.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <map>

#define parser Parser::instance()

class Parser {

	public:
		static Parser&						instance(void);

		const std::vector<Config>&			getAllConfigs(void) const;

		const Config& 						getConfig(size_t index) const;

		size_t								getNumConfigs(void) const;

		void								scan(const std::string& config_file);

		typedef void (Parser::*locationDirectiveHandler) (const std::string& val,
														Location& loc);

		typedef void (Parser::*serverDirectiveHandler) (const std::string& val,
														Config& config);

		typedef std::map<std::string,
						locationDirectiveHandler> locationDirectiveHandlerMap;

		typedef std::map<std::string,
						serverDirectiveHandler> serverDirectiveHandlerMap;

	private:
		Parser(void);
		Parser(const Parser& other);
		Parser& operator = (const Parser& other);
		~Parser(void);

		template<typename ConfigType>
		void _handleRoot(const std::string& val, ConfigType& config) {

			if (val.empty()) {
				throw std::runtime_error("parse error: root directive requires a value");
			}

			if (val[0] != '/') {
				throw std::runtime_error("parse error: root path must start with '/': " + val);
			}

			if (val[val.size() - 1] == '/') {
				throw std::runtime_error("parse error: root path must not end with '/': " + val);
			}

			config.root = val;

		}

		template<typename ConfigType>
		void _handleIndexFile(const std::string& val, ConfigType& config) {

			if (val.empty()) {
				throw std::runtime_error("parse error: index_files directive requires at least one file");
			}

			std::istringstream iss(val);
			std::string path;

			while (iss >> path) {

				// if (val.empty()) {
				// 	throw std::runtime_error("parse error: index file directive requires a value");
				// } // redundant, see "if (!(iss >> path))"

				if (val[0] == '/') {
					throw std::runtime_error("parse error: index file path must not start with '/': " + val);
				}

				if (val[val.size() - 1] == '/') {
					throw std::runtime_error("parse error: index file path must not end with '/': " + val);
				}

				// if (!isReadable(config.root + "/" + path)) {
				// 	throw std::runtime_error("parse error: no read access: " + path);
				// }

				config.index_files.push_back(path);

			}

		}

		template<typename ConfigType>
		void _handleErrorPage(const std::string& val, ConfigType& config) {

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
			if (code < 400 || 599 < code) {
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

			config.error_pages[code] = path;

			return;

		}

		static locationDirectiveHandlerMap _initLocationDirectiveHandlerMap(void);

		static serverDirectiveHandlerMap _initServerDirectiveHandlerMap(void);

		void	_parseLocationBlock(std::ifstream& config_file,
									Config& config,
									Location& loc);

		void	_parseServerBlock(std::ifstream& config_file);

		// Location directive handlers
		// void	_handleRoot(const std::string& val, Location& loc);
		void	_handleRedirect(const std::string& val, Location& loc);
		void	_handleAllowedMethods(const std::string& val, Location& loc);
		void	_handleAutoindex(const std::string& val, Location& loc);
		// void	_handleIndexFile(const std::string& val, Location& loc);
		void	_handleUploadDirectory(const std::string& val, Location& loc);
		// void	_handleCGIExt(const std::string& val, Location& loc);
		// void	_handleCGIPath(const std::string& val, Location& loc);
		void	_handleInterpreter(const std::string& val, Location& loc);
		// void	_handleErrorPage(const std::string& val, Location& loc);

		// Server directive handlers
		void	_handleListen(const std::string& val, Config& config);
		void	_handleHost(const std::string& val, Config& config);
		void	_handleServerNames(const std::string& val, Config& config);
		// void	_handleRoot(const std::string& val, Config& config);
		// void	_handleIndexFile(const std::string& val, Config& config);
		// void	_handleErrorPage(const std::string& val, Config& config);
		void	_handleClientMaxBodySize(const std::string& val, Config& config);

		std::vector<Config>		_configs;

};

// #include "Parser.tpp"

#endif
