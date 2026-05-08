#include "config_parser.hpp"

//-------------------------------Helper_Functions-------------------------------//
Config parse_config_file(const std::string& config_file)
{
	Config config;
	std::ifstream file(config_file);
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open config file: " << config_file << std::endl;
		exit(EXIT_FAILURE);
	}
	std::string line;
	std::vector<std::string> tokens;
	while(std::getline(file, line))
	{
		// Remove comments
		size_t comment_pos = line.find('#');
		if (comment_pos != std::string::npos)
			line = line.substr(0, comment_pos);
		// Trim whitespace
		line = trim(line);
		if (line.empty())
			continue;
		tokens.push_back(line);
	}

	return config;
}

//-------------------------------Constructors_&_Destructor-------------------------------//
config_parser::config_parser()
{
	this->config = parse_config_file("Config_Files/default.conf");
	return;
};


config_parser::config_parser(std::string config_file)
{
	this->config = parse_config_file("Config_Files/default.conf");
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