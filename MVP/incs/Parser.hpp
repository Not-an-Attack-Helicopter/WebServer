#pragma once

// #include "webserver.hpp"
#include "types.hpp"

#define parser Parser::instance()

class Parser {

	public:
		static Parser& instance(void);

		const std::vector<ServerConfig>&	getAllConfigs(void) const;

		const ServerConfig& 				getConfig(size_t index) const;

		size_t								countConfigs(void) const;

		void								readFile(const std::string& config);

	private:
		Parser(void);
		Parser(const Parser& other);
		Parser& operator=(const Parser& other);
		~Parser();

		std::vector<ServerConfig>			_serverConfigs;

};
