#pragma once

#include "webserver.hpp"

class config_parser
{
private:
	Config config;
public:
	config_parser();
	config_parser(std::string config_file);
	config_parser(const config_parser& other);
	config_parser& operator=(const config_parser& other);
	~config_parser();

	Config get_config() const;
};
