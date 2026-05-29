#include "utils.hpp"

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	size_t last = str.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

bool valid_ip(const std::string& ip)
{
	std::istringstream iss(ip);
	std::string token;
	int count = 0;
	while (std::getline(iss, token, '.'))
	{
		if (token.empty() || token.size() > 3)
			return false;
		for (size_t i = 0; i < token.size(); ++i)
		{
			if (!std::isdigit(token[i]))
				return false;
		}
		int num = std::atoi(token.c_str());
		if (num < 0 || num > 255)
			return false;
		count++;
	}
	return count == 4;
}

bool valid_port(const std::string& port_str)
{
	if (port_str.empty() || port_str.size() > 5)
		return false;
	for (size_t i = 0; i < port_str.size(); ++i)
	{
		if (!std::isdigit(port_str[i]))
			return false;
	}
	int port = std::atoi(port_str.c_str());
	return port > 0 && port <= 65535;
}

bool valid_config_line(const std::string& line)
{
    // List of valid keywords
    const char* kw[] = {
        "location", "listen", "host", "server_name", "root", "index", "error_page", "client_max_body_size", "}"
    };
    std::vector<std::string> valid_keywords(kw, kw + 9);

    if (line.empty())
        return true;
    if (line[0] == '#')
        return false;
    if (line[line.size() - 1] != ';')
        return false;

    std::string stripped = line.substr(0, line.size() - 1);

    size_t space_pos = stripped.find(' ');
    if (space_pos == std::string::npos)
        return false;

    std::string key = stripped.substr(0, space_pos);
    std::string value = stripped.substr(space_pos + 1);

    if (std::find(valid_keywords.begin(), valid_keywords.end(), key) == valid_keywords.end())
        return false;

    if (value.empty())
        return false;

    return true;
}



void print_conf(const Config& config) {
	for (std::vector<ServerConfig>::const_iterator server = config.servers.begin(); server != config.servers.end(); ++server) {
		std::cout << "server {\n";
		std::cout << "    host: " << server->host << ";\n";
		std::cout << "    port: " << server->port << ";\n";
		std::cout << "    client_max_body_size: " << server->client_max_body_size << ";\n";
		std::cout << "    error_pages {\n";
		for (std::map<int, std::string>::const_iterator error_page = server->error_pages.begin(); error_page != server->error_pages.end(); ++error_page) {
			std::cout << "        " << error_page->first << " " << error_page->second << ";\n";
		}
		std::cout << "    }\n";
		std::cout << "    locations {\n";
		for (std::vector<LocationConfig>::const_iterator location = server->locations.begin(); location != server->locations.end(); ++location) {
			std::cout << "        location " << location->path << " {\n";
			std::cout << "            methods: [";
			for (size_t i = 0; i < location->methods.size(); ++i) {
				std::cout << location->methods[i];
				if (i < location->methods.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            root: " << location->root << ";\n";
			std::cout << "            index: " << location->index << ";\n";
			std::cout << "            redirect: " << location->redirect << ";\n";
			std::cout << "            autoindex: " << (location->autoindex ? "on" : "off") << ";\n";
			std::cout << "            upload_dir: " << location->upload_dir << ";\n";
			std::cout << "            cgi_extension: [";
			for (size_t j = 0; j < location->cgi_extension.size(); ++j) {
				std::cout << location->cgi_extension[j];
				if (j < location->cgi_extension.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            cgi_path: [";
			for (size_t j = 0; j < location->cgi_path.size(); ++j) {
				std::cout << location->cgi_path[j];
				if (j < location->cgi_path.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "        }\n";
		}
		std::cout << "    }\n";
		std::cout << "}\n";
	}
}