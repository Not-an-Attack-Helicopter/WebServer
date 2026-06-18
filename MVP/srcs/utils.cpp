#include "../incs/utils.hpp"
#include "../incs/Logger.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <cstdio>

// DEBUG
void warnHighEventLoad(int nfds, int max_capacity) {
	int fill_percent = (nfds * 100) / max_capacity;

	if (fill_percent >= 95) {
		log.warning("Event array 95 full (" + i2a(nfds) + "/" + i2a(max_capacity) + ")");
		check_tcp_drops();
	}
	else if (fill_percent >= 75) {
		log.warning("Event array 75 full (" + i2a(nfds) + "/" + i2a(max_capacity) + ")");
		check_tcp_drops();
	}
	else if (fill_percent >= 60) {
		log.warning("Event array 60 full (" + i2a(nfds) + "/" + i2a(max_capacity) + ")");
		check_tcp_drops();
	}
	else if (fill_percent >= 50) {
		log.warning("Event array 50 full (" + i2a(nfds) + "/" + i2a(max_capacity) + ")");
		check_tcp_drops();
	}
}

void dumpEvents(int nfds, epoll_event* events) {
	log.debug("Total events: " + i2a(nfds));
	for (int i = 0; i < nfds; i++) {
		int fd = events[i].data.fd;
		log.debug("\tEvent " + i2a(i + 1) + " (fd_" + i2a(fd) + "):");
		if (events[i].events & EPOLLIN)		log.debug("\t\t\tEPOLLIN");
		if (events[i].events & EPOLLOUT)	log.debug("\t\t\tEPOLLOUT");
		if (events[i].events & EPOLLERR)	log.debug("\t\t\tEPOLLERR");
		if (events[i].events & EPOLLHUP)	log.debug("\t\t\tEPOLLHUP");
		log.notice("\n");
	}
}

static uint64_t last_backlog_drops = 0;
static uint64_t last_listen_drops = 0;
static uint64_t last_reqq_full_drops = 0;

void check_tcp_drops(void) {
	uint64_t current_backlog_drops = 0;
	uint64_t current_listen_drops = 0;
	uint64_t current_reqq_full_drops = 0;

	FILE *f = fopen("/proc/net/netstat", "r");
	if (!f) {
		perror("fopen(/proc/net/netstat)");
		return;
	}

	char line[4096];
	char headers[4096] = {0};
	char values[4096] = {0};

	// Read lines until we find TcpExt
	while (fgets(line, sizeof(line), f)) {
		if (strstr(line, "TcpExt") == line) {
			strcpy(headers, line);
			// Next line should contain the values
			if (fgets(line, sizeof(line), f)) {
				strcpy(values, line);
				break;
			}
		}
	}
	fclose(f);

	if (headers[0] == 0 || values[0] == 0) {
		// fprintf(stderr, "Could not find TcpExt in /proc/net/netstat\n");
		log.error("Could not find TcpExt in /proc/net/netstat");
		return;
	}

	// Parse headers to find column indices
	int backlog_col = -1, listen_col = -1, reqq_col = -1;
	int col = 0;
	char *token = strtok(headers, " ");

	while (token) {
		if (strcmp(token, "TCPBacklogDrop") == 0) backlog_col = col;
		if (strcmp(token, "ListenDrops") == 0) listen_col = col;
		if (strcmp(token, "TCPReqQFullDrop") == 0) reqq_col = col;
		token = strtok(NULL, " ");
		col++;
	}

	col = 0;
	token = strtok(values, " ");
	while (token) {
		if (col == backlog_col) current_backlog_drops = strtoull(token, NULL, 10);
		if (col == listen_col) current_listen_drops = strtoull(token, NULL, 10);
		if (col == reqq_col) current_reqq_full_drops = strtoull(token, NULL, 10);
		token = strtok(NULL, " ");
		col++;
	}

	log.warning("Total TCPBacklogDrops: " + i2a(current_backlog_drops));
	log.warning("Total ListenDrops: " + i2a(current_listen_drops));
	log.warning("Total TCPReqQFullDrops: " + i2a(current_reqq_full_drops));

	if (current_backlog_drops > 0) log.warning("Total TCPBacklogDrops: " + std::string(token));
	if (current_listen_drops > 0) log.warning("Total ListenDrops: " + std::string(token));
	if (current_reqq_full_drops > 0) log.warning("Total TCPReqQFullDrops: " + std::string(token));

	// Calculate drops since last check
	uint64_t new_backlog_drops = current_backlog_drops - last_backlog_drops;
	uint64_t new_listen_drops = current_listen_drops - last_listen_drops;
	uint64_t new_reqq_full_drops = current_reqq_full_drops - last_reqq_full_drops;

	log.error("TCPBacklogDrops increased by " + i2a(new_backlog_drops) + " since last check");
	log.error("ListenDrops increased by " + i2a(new_listen_drops) + " since last check");
	log.error("TCPReqQFullDrop increased by " + i2a(new_reqq_full_drops) + " since last check");

	if (new_backlog_drops > 0)
		log.error("TCPBacklogDrops increased by " + i2a(new_backlog_drops) + " since last check");
	if (new_listen_drops > 0)
		log.error("ListenDrops increased by " + i2a(new_listen_drops) + " since last check");
	if (new_reqq_full_drops > 0)
		log.error("TCPReqQFullDrop increased by " + i2a(new_reqq_full_drops) + " since last check");

	// Update baseline for next check
	last_backlog_drops = current_backlog_drops;
	last_listen_drops = current_listen_drops;
	last_reqq_full_drops = current_reqq_full_drops;
}
// DEBUG

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
			if (!std::isdigit(static_cast<unsigned char>(token[i])))
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
		if (!std::isdigit(static_cast<unsigned char>(port_str[i])))
			return false;
	}
	int port = std::atoi(port_str.c_str());
	return port > 0 && port <= 65535;
}

bool is_address_already_used(const std::vector<Config>& config, const std::string& host, int port)
{
	for (size_t i = 0; i < config.size(); ++i)
	{
		if (config[i].port == port && config[i].host == host)
			return true;
	}
	return false;
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

bool is_valid_method(const std::string& method)
{
	const std::string valid_methods[] = {"GET", "POST", "PUT", "HEAD", "DELETE"};
	const size_t s = sizeof(valid_methods) / sizeof(valid_methods[0]);
	return (std::find(valid_methods, valid_methods + s, method) != valid_methods + s);
}

bool is_valid_body_size(const int size)
{
	return size >= 0;
}

bool is_valid_error_code(const int code)
{
	return code >= 400 && code < 600;
}

// std::string i2a(short input) {
// 	std::stringstream convert;
// 	convert << input;
// 	return (convert.str());
// }

std::string i2a(unsigned short input) {
	std::stringstream convert;
	convert << input;
	return (convert.str());
}

std::string i2a(int input) {
	std::stringstream convert;
	convert << input;
	return (convert.str());
}

// std::string i2a(const int input) {
// 	std::stringstream convert;
// 	convert << input;
// 	return (convert.str());
// }

// std::string i2a(unsigned int input) {
// 	std::stringstream convert;
// 	convert << input;
// 	return (convert.str());
// }

std::string i2a(long input) {
	std::stringstream convert;
	convert << input;
	return (convert.str());
}

std::string i2a(unsigned long input) {
	std::stringstream convert;
	convert << input;
	return (convert.str());
}

std::string i2a(double input) {
	std::stringstream convert;
	convert << input;
	return (convert.str());
}

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	size_t last = str.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

std::string get_content_type(const std::string& path)
{
	size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1)
		return "application/octet-stream";
	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm")
		return "text/html";
	else if (ext == ".css")
		return "text/css";
	else if (ext == ".js")
		return "application/javascript";
	else if (ext == ".jpg" || ext == ".jpeg")
		return "image/jpeg";
	else if (ext == ".png")
		return "image/png";
	else if (ext == ".gif")
		return "image/gif";
	else if (ext == ".py" || ext == ".sh")
		return "text/x-script";
	else
		return "application/octet-stream";
}

void print_conf(const std::vector<Config>& config) {
	for (std::vector<Config>::const_iterator server = config.begin(); server != config.end(); ++server) {
		std::cout << "server {\n";
		std::cout << "    host: " << server->host << ";\n";
		std::cout << "    port: " << server->port << ";\n";
		std::cout << "    root: " << server->root << ";\n";
		std::cout << "    index: " << server->index << ";\n";
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
