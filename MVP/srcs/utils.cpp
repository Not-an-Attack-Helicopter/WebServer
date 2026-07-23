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
#include <sys/stat.h>	// stat
#include <unistd.h>
#include <iostream>
#include <climits>		// for USHRT_MAX, INT_MIN, INT_MAX
#include <cstring>
#include <cstdlib>
#include <cstdio>

// DEBUG BEGIN
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

	size_t j;
	size_t k;
	const Config& c = client->getConfig();
	const std::vector<std::string> n = c.server_names;
	const std::vector<Location> l = c.locations;
	const std::vector<std::string> i = c.index_files;

	log.info(c.host + " " + i2a(c.port) + " " + c.root + " ");
	j = -1;
	while (++j < i.size()) {
		log.info(i[j]);
	}
	j = -1;
	while (++j < n.size()) {
		log.info(n[j]);
	}
	j = -1;
	k = -1;
	while (++j < l.size()) {
		log.info(l[j].path + " " + l[j].root + " ");
		while (++k < l[j].index_files.size()) {
			log.info(l[j].index_files[j] + " ");
		}
		log.info(l[j].redirect + " " + i2a(l[j].autoindex) + " " + l[j].upload_dir);
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

	// Print all headers
	// std::map<std::string, std::string>::iterator it = request->getHeaders().begin();
	// while (it != request->getHeaders().end()) {
	// 	log.debug(it->first + ":\t" + it->second);
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
	if (!request->getBodyPath().empty())
		log.debug("Body file:\t" + request->getBodyPath());
}
// DEBUG END

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

bool isRegularFile(const std::string& path) {

	// if (access(path.c_str(), F_OK) == -1) {
	// 	// return false;
	// 	log.error("F_KO");
	// }

	// log.error(path);
	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) {
	// int status = stat(path.c_str(), &sb);
	// log.error(i2a(status));
	// if (status != 0) {
		log.error("FILE cassé");
		return false;
	}

	return S_ISREG(sb.st_mode);

}

bool isDirectory(const std::string& path) {

	// if (access(path.c_str(), F_OK) == -1) {
	// 	// return false;
	// 	log.error("F_KO");
	// }

	// log.error(path);
	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) {
	// int status = stat(path.c_str(), &sb);
	// log.error(i2a(status));
	// if (status != 0) {
		log.error("DIR cassé");
		return false;
	}

	return S_ISDIR(sb.st_mode);

}

// bool isReadable(const std::string& path) {
//
// 	if (!isRegularFile(path)) {
// 		return false;
// 	}
//
// 	return access(path.c_str(), R_OK) == 0;
//
// }

// bool isValidErrorCode(const int code) {
//
// 	return code >= 400 && code < 600;
//
// }

void dumpConfigs(const std::vector<Config>& config) {
	for (std::vector<Config>::const_iterator server = config.begin(); server != config.end(); ++server) {
		std::cout << "server {\n";
		std::cout << "    host: " << server->host << ";\n";
		std::cout << "    port: " << server->port << ";\n";
		std::cout << "    server_names: [";
		for (size_t j = 0; j < server->server_names.size(); ++j) {
			std::cout << server->server_names[j];
			if (j < server->server_names.size() - 1) std::cout << ", ";
		}
		std::cout << "];\n";
		std::cout << "    root: " << server->root << ";\n";
		std::cout << "    index_files: [";
		for (size_t j = 0; j < server->index_files.size(); ++j) {
			std::cout << server->index_files[j];
			if (j < server->index_files.size() - 1) std::cout << ", ";
		}
		std::cout << "];\n";
		std::cout << "    error_pages {\n";
		for (std::map<int, std::string>::const_iterator error_page = server->error_pages.begin(); error_page != server->error_pages.end(); ++error_page) {
			std::cout << "        " << error_page->first << " " << error_page->second << ";\n";
		}
		std::cout << "    }\n";
		std::cout << "    client_max_body_size: " << server->client_max_body_size << ";\n";
		std::cout << "    locations {\n";
		for (std::vector<Location>::const_iterator location = server->locations.begin(); location != server->locations.end(); ++location) {
			std::cout << "        location " << location->path << " {\n";
			std::cout << "            root: " << location->root << ";\n";
			std::cout << "            redirect: " << location->redirect << ";\n";
			std::cout << "            methods: [";
			for (size_t i = 0; i < location->methods.size(); ++i) {
				std::cout << location->methods[i];
				if (i < location->methods.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            autoindex: " << (location->autoindex ? "on" : "off") << ";\n";
			// std::cout << "            index: " << location->index << ";\n";
			std::cout << "            index files: [";
			for (size_t j = 0; j < location->index_files.size(); ++j) {
				std::cout << location->index_files[j];
					if (j < location->index_files.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            upload_dir: " << location->upload_dir << ";\n";
			// std::cout << "            cgi_extension: [";
			// for (size_t j = 0; j < location->cgi_extensions.size(); ++j) {
			// 	std::cout << location->cgi_extensions[j];
			// 	if (j < location->cgi_extensions.size() - 1) std::cout << ", ";
			// }
			// std::cout << "];\n";
			// std::cout << "            cgi_path: [";
			// for (size_t j = 0; j < location->cgi_paths.size(); ++j) {
			// 	std::cout << location->cgi_paths[j];
			// 	if (j < location->cgi_paths.size() - 1) std::cout << ", ";
			// }
			// std::cout << "];\n";
			std::cout << "            interpreter: {\n";
			for (std::map<std::string, std::string>::const_iterator it = location->interpreters.begin();
				 it != location->interpreters.end(); ++it) {
				std::cout << "            " + it->first + " " + it->second << ";\n";
			}
			std::cout << "            };\n";
			std::cout << "                error_pages {\n";
			for (std::map<int, std::string>::const_iterator error_page = location->error_pages.begin();
				 error_page != location->error_pages.end();
				 ++error_page) {
				std::cout << "            " << error_page->first << " " << error_page->second << ";\n";
			}
			std::cout << "            }\n";
			std::cout << "        }\n";
		}
		std::cout << "    }\n";
		std::cout << "}\n";
	}
	return;
}
