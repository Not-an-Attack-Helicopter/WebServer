#include "parse_helper.hpp"


static std::string directive_key(const std::string& line)
{
	size_t space = line.find(' ');
	if (space == std::string::npos)
		return line;
	return line.substr(0, space);
}


static std::string directive_value(const std::string& line)
{
	size_t space = line.find(' ');
	if (space == std::string::npos)
		return "";
	std::string val = line.substr(space + 1);
	if (!val.empty() && val[val.size() - 1] == ';')
		val = val.substr(0, val.size() - 1);
	return trim(val);
}

static std::vector<std::string> Tokenizer(const std::string& config_file)
{
	std::ifstream file(config_file.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open config file: " << config_file << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string content;
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line = line.substr(0, line.size() - 1);
		size_t comment_pos = line.find('#');
		if (comment_pos != std::string::npos)
			line = line.substr(0, comment_pos);
		content += line + " ";
	}

	std::vector<std::string> tokens;
	std::string current;
	for (size_t i = 0; i < content.size(); ++i)
	{
		char c = content[i];
		if (c == '{')
		{
			std::string tok = trim(current) + " {";
			if (trim(current).empty())
			{
				std::cerr << "Error: unexpected '{' with no preceding keyword" << std::endl;
				exit(EXIT_FAILURE);
			}
			tokens.push_back(trim(tok));
			current.clear();
		}
		else if (c == '}')
		{
			std::string leftover = trim(current);
			if (!leftover.empty())
			{
				std::cerr << "Error: missing semicolon: " << leftover << std::endl;
				exit(EXIT_FAILURE);
			}
			tokens.push_back("}");
			current.clear();
		}
		else if (c == ';')
		{
			std::string tok = trim(current);
			if (!tok.empty())
				tokens.push_back(tok + ";");
			current.clear();
		}
		else
		{
			current += c;
		}
	}
	std::string leftover = trim(current);
	if (!leftover.empty())
	{
		std::cerr << "Error: unexpected content at end of file (missing '}' or ';'?): " << leftover << std::endl;
		exit(EXIT_FAILURE);
	}
	return tokens;
}

LocationConfig parse_location_block(const std::vector<std::string>& tokens, size_t& i)
{
	LocationConfig loc;
	loc.autoindex = false;

	const std::string& header = tokens[i];
	size_t first_space = header.find(' ');
	size_t last_space  = header.rfind(' ');
	if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space)
		loc.path = trim(header.substr(first_space + 1, last_space - first_space - 1));
	else
		loc.path = "/";

	++i;

	while (i < tokens.size() && tokens[i] != "}")
	{
		std::string key = directive_key(tokens[i]);
		std::string val = directive_value(tokens[i]);

		if (key == "allow_methods")
		{
			std::istringstream iss(val);
			std::string method;
			while (iss >> method)
				loc.methods.push_back(method);
		}
		else if (key == "autoindex")
			loc.autoindex = (val == "on");
		else if (key == "index")
			loc.index = val;
		else if (key == "root")
			loc.root = val;
		else if (key == "return")
			loc.redirect = val;
		else if (key == "upload_dir")
			loc.upload_dir = val;
		else if (key == "cgi_ext")
		{
			if(!valid_CGI_ext(val))
			{
				std::cerr << "Error: invalid CGI extension: " << val << std::endl;
				exit(EXIT_FAILURE);
			}
			loc.cgi_extension = val;
		}
		else if (key == "cgi_path")
		{	
			if(!valid_CGI(val))
			{
				std::cerr << "Error: invalid CGI path: " << val << std::endl;
				exit(EXIT_FAILURE);
			}
			loc.cgi_path = val;
		}
		else
		{
			std::cerr << "Error: unknown directive in location block: " << key << std::endl;
			exit(EXIT_FAILURE);
		}

		++i;
	}
	if (i >= tokens.size())
	{
		std::cerr << "Error: unclosed location block '" << loc.path << "' (missing '}}')" << std::endl;
		exit(EXIT_FAILURE);
	}
	++i;
	return loc;
}


ServerConfig parse_server_block(const std::vector<std::string>& tokens, size_t& i)
{
	ServerConfig server;
	server.port = 80;
	server.client_max_body_size = 1048576;

	++i;

	while (i < tokens.size() && tokens[i] != "}")
	{
		std::string key = directive_key(tokens[i]);
		std::string val = directive_value(tokens[i]);

		if (key == "listen")
		{
			if (!valid_port(val))
			{
				std::cerr << "Error: invalid port value: " << val << std::endl;
				exit(EXIT_FAILURE);
			}
			server.port = std::atoi(val.c_str());
		}
		else if (key == "host")
		{
			if (!valid_ip(val))
			{
				std::cerr << "Error: invalid host IP: " << val << std::endl;
				exit(EXIT_FAILURE);
			}
			server.host = val;
		}
		else if (key == "server_name")
		{
			std::istringstream iss(val);
			std::string name;
			while (iss >> name)
				server.server_names.push_back(name);
		}
		else if (key == "client_max_body_size")
			server.client_max_body_size = (size_t)std::atol(val.c_str());
		else if (key == "error_page")
		{
			std::istringstream iss(val);
			std::string code_str;
			std::string path;
			if (iss >> code_str >> path)
				server.error_pages[std::atoi(code_str.c_str())] = path;
		}
		else if (key == "location")
		{

			server.locations.push_back(parse_location_block(tokens, i));
			continue;
		}
		else if (key != "root" && key != "index")
		{
			std::cerr << "Error: unknown directive in server block: " << key << std::endl;
			exit(EXIT_FAILURE);
		}

		++i;
	}
	if (i >= tokens.size())
	{
		std::cerr << "Error: unclosed server block (missing '}')" << std::endl;
		exit(EXIT_FAILURE);
	}
	++i;
	return server;
}

Config parse_config_file(const std::string& config_file)
{
	if(!valid_conf_ext(config_file))
	{
		std::cerr << "Error: invalid config file extension: " << config_file << std::endl;
		exit(EXIT_FAILURE);
	}
	std::vector<std::string> tokens = Tokenizer(config_file);
	Config config;

	for (size_t i = 0; i < tokens.size(); )
	{
		if (tokens[i] == "server {")
			config.servers.push_back(parse_server_block(tokens, i));
		else
		{
			std::cerr << "Error: unexpected token outside server block: " << tokens[i] << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	return config;
}

bool valid_conf_ext(const std::string& filename)
{
	size_t dot = filename.rfind('.');
	if (dot == std::string::npos || dot == filename.size() - 1)
		return false;
	std::string ext = filename.substr(dot);
	return (ext == ".conf");
}


bool valid_CGI_ext(const std::string& path)
{
	const std::string valid_exts[] = {".py", ".php"};
	const size_t s = sizeof(valid_exts) / sizeof(valid_exts[0]);
	size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1)
		return false;
	std::string ext = path.substr(dot);
	return (std::find(valid_exts, valid_exts + s, ext) != valid_exts + s);
}

bool valid_CGI(const std::string& path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && (buffer.st_mode & S_IXUSR));
}