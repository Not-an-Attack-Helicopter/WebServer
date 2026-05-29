#include "config_parser.hpp"

//-------------------------------Constructors_&_Destructor-------------------------------//
config_parser::config_parser()
{
	this->config = parse_config_file("Config_Files/default.conf");
	return;
};


config_parser::config_parser(std::string config_file)
{
	this->config = parse_config_file(config_file);
	return;
};


config_parser::config_parser(const config_parser& other)
{
	this->config = other.config;
	return;
};


config_parser& config_parser::operator=(const config_parser& other)
{
	if (this != &other)
	{
		this->config = other.config;
	}
	return *this;
};


config_parser::~config_parser()
{
	return;
};


//-------------------------------getters-------------------------------//
Config config_parser::get_config() const
{
	return config;
}