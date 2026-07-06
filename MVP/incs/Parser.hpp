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
#include <cstddef>
#include <string>
#include <vector>
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
														LocationConfig& loc);

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

		static locationDirectiveHandlerMap _initLocationDirectiveHandlerMap(void);

		static serverDirectiveHandlerMap _initServerDirectiveHandlerMap(void);

		void	_parseLocationBlock(std::ifstream& config_file,
									Config& config,
									LocationConfig& loc);

		void	_parseServerBlock(std::ifstream& config_file);

		// Location directive handlers
		void	_handleAllowedMethods(const std::string& val, LocationConfig& loc);
		void	_handleAutoindex(const std::string& val, LocationConfig& loc);
		void	_handleIndex(const std::string& val, LocationConfig& loc);
		void	_handleRoot(const std::string& val, LocationConfig& loc);
		void	_handleReturn(const std::string& val, LocationConfig& loc);
		void	_handleUploadDir(const std::string& val, LocationConfig& loc);
		void	_handleCGIExt(const std::string& val, LocationConfig& loc);
		void	_handleCGIPath(const std::string& val, LocationConfig& loc);

		// Server directive handlers
		void	_handleListen(const std::string& val, Config& config);
		void	_handleHost(const std::string& val, Config& config);
		void	_handleServerNames(const std::string& val, Config& config);
		void	_handleClientMaxBodySize(const std::string& val, Config& config);
		void	_handleErrorPage(const std::string& val, Config& config);
		void	_handleServerRoot(const std::string& val, Config& config);
		void	_handleServerIndex(const std::string& val, Config& config);

		std::vector<Config>		_configs;

};

#endif
