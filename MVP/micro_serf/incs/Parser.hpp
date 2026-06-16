#pragma once

// #include "webserver.hpp"
#include "types.hpp"

class Parser {

	public:
		// Parser();
		Parser(std::string config_file);
		Parser(const Parser& other);
		Parser& operator=(const Parser& other);
		~Parser();

		const std::vector<ServerConfig>&	getAllConfigs(void) const;
		const ServerConfig& 				getConfig(size_t index) const;

		size_t								countConfigs(void) const;

	private:
		std::vector<ServerConfig>			_serverConfigs;

};
