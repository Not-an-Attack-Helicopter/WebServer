#pragma once

#include "webserver.hpp"

class ConfigParser
{
private:
	Config config;
public:
	ConfigParser();
	ConfigParser(std::string config_file);
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	Config                  get_config() const;
	const Config&           getAllConfigs() const;
	const ServerConfig&     getSingleConfig(size_t index) const;
	size_t                  getServerConfigCount() const;
};
