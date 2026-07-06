#include "config_parser.hpp"

//-------------------------------Constructors_&_Destructor-------------------------------//
ConfigParser::ConfigParser()
{
	this->config = parse_config_file("Config_Files/default.conf");
	return;
};


ConfigParser::ConfigParser(std::string config_file)
{
	this->config = parse_config_file(config_file);
	return;
};


ConfigParser::ConfigParser(const ConfigParser& other)
{
	this->config = other.config;
	return;
};


ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		this->config = other.config;
	}
	return *this;
};


ConfigParser::~ConfigParser()
{
	return;
};


//-------------------------------getters-------------------------------//
Config ConfigParser::get_config() const
{
	return config;
}

const Config& ConfigParser::getAllConfigs() const
{
	return config;
}

const ServerConfig& ConfigParser::getSingleConfig(size_t index) const
{
	return config.servers[index];
}

size_t ConfigParser::getServerConfigCount() const
{
	return config.servers.size();
}