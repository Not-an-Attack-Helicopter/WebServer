/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:36:42 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/30 18:36:43 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/utils.hpp"
#include "../incs/Logger.hpp"
#include "../incs/templates.hpp"
#include "../incs/HTTPRequest.hpp"
#include <climits>   // for USHRT_MAX, INT_MIN, INT_MAX
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

// DEBUG >>
void warnHighEventLoad(int nfds, int max_capacity) {
	int utilization = (nfds * 100) / max_capacity;
	if (utilization >= 95) {
		log.warn("Event array (over) 95 percent full ("
			+ i2a(nfds) + "/" + i2a(max_capacity) + ")");
	}
	else if (utilization >= 85) {
		log.warn("Event array (over) 85 percent full ("
			+ i2a(nfds) + "/" + i2a(max_capacity) + ")");
	}
	else if (utilization >= 70) {
		log.warn("Event array (over) 70 percent full ("
			+ i2a(nfds) + "/" + i2a(max_capacity) + ")");
	}
	else if (utilization >= 50) {
		log.warn("Event array (over) 50 percent full ("
			+ i2a(nfds) + "/" + i2a(max_capacity) + ")");
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
	}
}

void dumpClientConfig(const Client* client) {

	size_t i;;
	const Config& c = client->getConfig();
	const std::vector<std::string> n = c.server_names;
	const std::vector<LocationConfig> l = c.locations;

	log.info(c.host + " " + i2a(c.port) + " " + c.root + " " + c.index);
	i = -1;
	while (++i < n.size()) {
		log.info(n[i]);
	}
	i = -1;
	while (++i < l.size()) {
		log.info(l[i].path + " " + l[i].root + " " + l[i].index + " " + l[i].redirect + " " + i2a(l[i].autoindex) + " " + l[i].upload_dir);
	}
}

void dumpRequest(HTTPRequest* request) {
	std::string state;
	switch(request->getState()) {
		case PS_READING_REQUEST_LINE: state = "reading request line"; break;
		case PS_READING_HEADERS: state = "reading headers"; break;
		case PS_READING_BODY: state = "reading body"; break;
		case PS_COMPLETE: state = "complete"; break;
		case PS_ERROR: state = "error"; break;
	}
	log.debug("State:\t\t" + state + " (" + i2a(request->getState()) + ")");
	// log.debug("State:\t\t" + i2a(request.getState()));
	log.debug("Method:\t\t" + request->getMethod());
	log.debug("Path:\t\t" + request->getPath());
	log.debug("Query:\t\t" + request->getQuery());
	log.debug("Version:\t" + request->getVersion());

	// std::map<std::string, std::string>::iterator it = request->getHeaders().begin();
	// while (it != request->getHeaders().end()) {
	// 	log.debug(it->first + ":\t\t\t" + it->second);
	// 	++it;
	// }

	if (request->hasHeader("host"))
		log.debug("Host:\t\t" + request->getHeader("host"));
	if (request->hasHeader("user-agent"))
		log.debug("User-Agent:\t" + request->getHeader("user-agent"));
	if (request->hasHeader("accept"))
		log.debug("Accept:\t\t" + request->getHeader("accept"));
	if (request->hasHeader("connection"))
		log.debug("Connection:\t" + request->getHeader("connection"));
	if (request->hasHeader("content-type"))
		log.debug("Content-Type:\t" + request->getHeader("content-type"));
	if (request->hasHeader("content-length"))
		log.debug("Content-Length:\t" + request->getHeader("content-length"));
		// log.debug("Content-Length:\t" + i2a(request->getContentLength()) + "\n");

	log.debug("Body:\t\t" + request->getBody());
}
// << DEBUG

unsigned short stringToUnsignedShort(const std::string& str) {

	char* endptr;
	unsigned long tmp = std::strtoul(str.c_str(), &endptr, 10);

	if (*endptr != '\0') {
		throw std::runtime_error("type conversion failed: " + str);
	}

	if (tmp > USHRT_MAX) {
		throw std::out_of_range("type conversion failed (value out of range for unsigned short): " + str);
	}

	return static_cast<unsigned short>(tmp);

}

size_t stringToSize(const std::string& str) {

	char* endptr;
	unsigned long tmp = std::strtoul(str.c_str(), &endptr, 10);

	if (*endptr != '\0') {
		throw std::runtime_error("type conversion failed: " + str);
	}
	return static_cast<size_t>(tmp);

}

int stringToInt(const std::string& str) {

	if (str.empty()) {
		throw std::runtime_error("type conversion failed: cannot convert empty string to int");
	}

	char* endptr;
	long tmp = std::strtol(str.c_str(), &endptr, 10);

	if (*endptr != '\0') {
		throw std::runtime_error("type conversion failed (invalid characters): " + str);
	}

	if (tmp < INT_MIN || tmp > INT_MAX) {
		throw std::out_of_range("type conversion failed (value out of range for int): " + str);
	}

	return static_cast<int>(tmp);
}

std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	size_t last = str.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

std::string get_content_type(const std::string& path) {
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

void dumpConfigs(const std::vector<Config>& config) {
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
			for (size_t j = 0; j < location->cgi_extensions.size(); ++j) {
				std::cout << location->cgi_extensions[j];
				if (j < location->cgi_extensions.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            cgi_path: [";
			for (size_t j = 0; j < location->cgi_paths.size(); ++j) {
				std::cout << location->cgi_paths[j];
				if (j < location->cgi_paths.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "        }\n";
		}
		std::cout << "    }\n";
		std::cout << "}\n";
	}
}
