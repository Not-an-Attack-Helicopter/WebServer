#include "../incs/ConfigParser.hpp"
#include "../incs/helpers.hpp"
#include <cstddef>
// #include <iostream>

//-------------------------------Constructors_&_Destructor-------------------------------//
// ConfigParser::ConfigParser()
// {
// 	this->_serverConfigs = parse_config_file("Config_Files/default.conf");
// 	return;
// };


ConfigParser::ConfigParser(std::string config_file)
{
	this->_serverConfigs = parse_config_file(config_file);
	// std::cout << "Parser constructed" << std::endl;
	return;
};


ConfigParser::ConfigParser(const ConfigParser& other) : _serverConfigs(other._serverConfigs) {
	return;
};


ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		this->_serverConfigs = other._serverConfigs;
	}
	return *this;
};


ConfigParser::~ConfigParser()
{
	// std::cout << "Parser destructed" << std::endl;
	return;
};


//-------------------------------getters-------------------------------//
const std::vector<ServerConfig>& ConfigParser::getAllConfigs() const
{
	return _serverConfigs;
}


const ServerConfig& ConfigParser::getSingleConfig(size_t index) const {
	return _serverConfigs[index];
}

size_t ConfigParser::getServerConfigCount(void) const {
	return _serverConfigs.size();
}
