#pragma once

// #include "webserver.hpp"
#include "types.hpp"

class ConfigParser {

	public:
		// ConfigParser();
		ConfigParser(std::string config_file);
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);
		~ConfigParser();

		// std::vector<ServerConfig>&			get_config(void);
		const ServerConfig& 				getServerConfig(size_t index) const;

		size_t								getServerConfigCount(void) const;

	private:
		std::vector<ServerConfig>			_serverConfigs;

};
