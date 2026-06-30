/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz <marvin@42.fr>                       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:41:59 by sholz             #+#    #+#             */
/*   Updated: 2026/06/30 18:42:00 by sholz            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

// #include "webserver.hpp"
#include "types.hpp"

#define parser Parser::instance()

class Parser {

	public:
		static Parser& 						instance(void);

		const std::vector<Config>&			getAllConfigs(void) const;

		const Config& 						getConfig(size_t index) const;

		size_t								countConfigs(void) const;

		void								readFile(const std::string& config);

	private:
		Parser(void);
		Parser(const Parser& other);
		Parser& operator=(const Parser& other);
		~Parser(void);

		static std::string					_directiveKey(const std::string& line);
		static std::string					_directiveValue(const std::string& line);
		static std::vector<std::string>		_tokenizer(const std::string& config_file);
		LocationConfig						_parseLocationBlock(const std::vector<std::string>& tokens, size_t& i);
		Config								_parseServerBlock(const std::vector<std::string>& tokens, size_t& i, const std::vector<Config>& config);
		std::vector<Config>					_parseConfigFile(const std::string& config_file);
		bool								_validConfExt(const std::string& filename);
		bool								_validCGIExt(const std::string& ext);
		bool								_validCGI(const std::string& path);

		std::vector<Config>					_serverConfigs;

};

#endif
