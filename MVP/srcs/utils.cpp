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
#include <vector>

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
		if (events[i].events & EPOLLRDHUP)	log.debug("\t\t\tEPOLLRDHUP");
	}
}

void dumpClientConfig(const Client* client) {

	const Config::Socket& soc = client->getConfig();
	log.info(soc.address + " " + i2a(soc.port));
	for (size_t i = 0; i < soc.domains.size(); ++i) {
		for (size_t j = 0; j < soc.domains[i].names.size(); ++j) {
			log.info(soc.domains[i].names[j]);
		}
		log.info(soc.domains[i].root);
		for (size_t j = 0; j < soc.domains[i].index_files.size(); ++j) {
			log.info(soc.domains[i].index_files[j]);
		}
		for (size_t j = 0; j < soc.domains[i].locations.size(); ++j) {
			log.info(soc.domains[i].locations[j].path);
			log.info(soc.domains[i].locations[j].root);
			// log.info(soc.domains[i].locations[j].redirect);
			// log.info(soc.domains[i].locations[j].upload_dir);
			for (size_t k = 0; k < soc.domains[i].locations[j].index_files.size(); ++k) {
				log.info(soc.domains[i].locations[j].index_files[k]);
			}
		}
	}
}

void dumpRequest(HTTPRequest* request) {
	std::string parsing_state;
	switch(request->parsing.state) {
		case HTTPRequest::READING_REQUEST_LINE: parsing_state = "reading request line"; break;
		case HTTPRequest::READING_HEADERS: parsing_state = "reading headers"; break;
		case HTTPRequest::READING_BODY: parsing_state = "reading body"; break;
		case HTTPRequest::DISPATCHING: parsing_state = "dispatching"; break;
		// case HTTPRequest::FINALIZING: parsing_state = "finalizing"; break;
		case HTTPRequest::COMPLETE: parsing_state = "complete"; break;
		case HTTPRequest::ERROR: parsing_state = "error"; break;
	}
	log.debug("State:\t\t" + parsing_state + " (" + i2a(request->parsing.state) + ")");
	// log.debug("State:\t\t" + i2a(request.getState()));
	log.debug("Method:\t\t" + request->getMethodName());
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

	// log.debug("Body:\t\t" + request->getBody().str());
}
// DEBUG END

unsigned short stringToUnsignedShort(const std::string& str) {

	if (str.empty()) {
		throw std::runtime_error("type conversion failed: cannot convert empty string to int");
	}

	if (str[0] == '-') {
		throw std::runtime_error("type conversion failed: invalid negative value for unsigned type");
	}

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

// long stringToLong(const std::string& str) {
//
// 	if (str.empty()) {
// 		throw std::runtime_error("type conversion failed: cannot convert empty string to int");
// 	}
//
// 	char* endptr;
// 	long tmp = std::strtol(str.c_str(), &endptr, 10);
//
// 	if (*endptr != '\0') {
// 		throw std::runtime_error("type conversion failed (invalid characters): " + str);
// 	}
// 	return tmp;
//
// }

size_t stringToSize(const std::string& str) {

	if (str.empty()) {
		throw std::runtime_error("type conversion failed: cannot convert empty string to int");
	}

	if (str[0] == '-') {
		throw std::runtime_error("type conversion failed: invalid negative value for unsigned type");
	}

	char* endptr;
	unsigned long tmp = std::strtoul(str.c_str(), &endptr, 10);

	if (*endptr != '\0') {
		throw std::runtime_error("type conversion failed: " + str);
	}

	if (tmp > ULONG_MAX) {
		throw std::out_of_range("type conversion failed (value out of range for unsigned long): " + str);
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

std::string randomHexString(size_t n) {

	static const char hex[] = "0123456789abcdef";

	unsigned char* bytes = new unsigned char[n];

	try {
		std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);

		if (!urandom) {
			throw std::runtime_error("cannot open /dev/urandom");
		}

		urandom.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(n));

		if (urandom.gcount() != static_cast<std::streamsize>(n)) {
			throw std::runtime_error("cannot read /dev/urandom");
		}

		std::string result;
		result.reserve(n * 2);

		for (std::size_t i = 0; i < n; ++i) {
			result += hex[bytes[i] >> 4];
			result += hex[bytes[i] & 0x0f];
		}

		delete [] bytes;
		return result;
	}
	catch (std::exception& e) {
		delete [] bytes;
		log.error("hexgen: " + std::string(e.what()));
		return i2a(std::time(NULL) * errno == 0 ? 1 : errno);
	}
}

bool isRegularFile(const std::string& path) {

	// if (access(path.c_str(), F_OK) == -1) {
	// 	// return false;
	// 	log.error("F_KO");
	// }

	// log.error(path);
	// struct stat sb;
	// if (stat(path.c_str(), &sb) != 0) {
	// int status = stat(path.c_str(), &sb);
	// log.error(i2a(status));
	// if (status != 0) {
	// 	// log.error("FILE cassé");
	// 	return false;
	// }

	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) return false;
	return S_ISREG(sb.st_mode);

}

bool isDirectory(const std::string& path) {

	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) return false;
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

// for (std::vector<Config::Socket>::const_iterator soc_it = sockets.begin(); soc_it != sockets.end(); ++soc_it) {
// for (std::vector<Config::Domain>::const_iterator dom_it = soc_it->domains.begin(); dom_it != soc_it->domains.end(); ++dom_it) {
// for (std::vector<Config::Location>::const_iterator location = server->locations.begin(); location != server->locations.end(); ++location) {

void dumpConfigs(const std::vector<Config::Socket>& sockets) {
	for (size_t i = 0; i < sockets.size(); ++i) {
		std::cout << "socket {\n";
		std::cout << "\thost: " << sockets[i].address << ";\n";
		std::cout << "\tport: " << sockets[i].port << ";\n";
		std::cout << "\tclient_max_body_size: " << sockets[i].client_max_body_size << ";\n";
		for (size_t j = 0; j < sockets[i].domains.size(); ++j) {
			std::cout << "\tdomain {\n";
			std::cout << "\t\tnames: [";
			for (size_t k = 0; k < sockets[i].domains[j].names.size(); ++k) {
				std::cout << sockets[i].domains[j].names[k];
				if (k < sockets[i].domains[j].names.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "\t\troot: " << sockets[i].domains[j].root << ";\n";
			std::cout << "\t\tindex_files: [";
			for (size_t k = 0; k < sockets[i].domains[j].index_files.size(); ++k) {
				std::cout << sockets[i].domains[j].index_files[k];
				if (k < sockets[i].domains[j].index_files.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "\t\terror_pages {\n";
			for (std::map<int, std::string>::const_iterator error_page = sockets[i].domains[j].error_pages.begin();
				 error_page != sockets[i].domains[j].error_pages.end(); ++error_page) {
				std::cout << "\t\t\t" << error_page->first << " " << error_page->second << ";\n";
			}
			std::cout << "\t\t}\n";
			std::cout << "\t\tclient_max_body_size: " << sockets[i].domains[j].client_max_body_size << ";\n";
			for (size_t k = 0; k < sockets[i].domains[j].locations.size(); ++k) {
				std::cout << "\t\tlocation {\n";
				std::cout << "\t\t\tpath " << sockets[i].domains[j].locations[k].path << ";\n";
				std::cout << "\t\t\troot: " << sockets[i].domains[j].locations[k].root << ";\n";
				std::cout << "\t\t\talias: " << sockets[i].domains[j].locations[k].alias << ";\n";
				std::cout << "\t\t\tredirect: " << sockets[i].domains[j].locations[k].redirect << ";\n";
				std::cout << "\t\t\tmethods: [";
				for (size_t l = 0; l < sockets[i].domains[j].locations[k].methods.size(); ++l) {
					std::string method;
					switch(sockets[i].domains[j].locations[k].methods[l]) {
						case GET: method = "GET"; break;
						case POST: method = "POST"; break;
						case DELETE: method = "DELETE"; break;
						case HEAD: method = "HEAD"; break;
						default: method = "N/A"; break;
					}
					std::cout << method;
					if (l < sockets[i].domains[j].locations[k].methods.size() - 1) std::cout << ", ";
				}
				std::cout << "];\n";
				std::cout << "\t\t\tautoindex: " << (sockets[i].domains[j].locations[k].autoindex ? "on" : "off") << ";\n";
				std::cout << "\t\t\tindex files: [";
				for (size_t l = 0; l < sockets[i].domains[j].locations[k].index_files.size(); ++l) {
					std::cout << sockets[i].domains[j].locations[k].index_files[l];
						if (l < sockets[i].domains[j].locations[k].index_files.size() - 1) std::cout << ", ";
				}
				std::cout << "];\n";
				std::cout << "\t\t\terror_pages {\n";
				for (std::map<int, std::string>::const_iterator error_page = sockets[i].domains[j].locations[k].error_pages.begin();
					 error_page != sockets[i].domains[j].locations[k].error_pages.end(); ++error_page) {
					std::cout << "\t\t\t\t" << error_page->first << " " << error_page->second << ";\n";
				}
				std::cout << "\t\t\t}\n";
				std::cout << "\t\t\tupload_dir: " << sockets[i].domains[j].locations[k].upload_dir << ";\n";
				std::cout << "\t\t\tclient_max_body_size: " << sockets[i].domains[j].locations[k].client_max_body_size << ";\n";
				// std::cout << "\t\t\tcgi_extension: [";
				// for (size_t l = 0; l < sockets[i].domains[j].locations[k].cgi_extensions.size(); ++l) {
				// 	std::cout << sockets[i].domains[j].locations[k].cgi_extensions[l];
				// 	if (l < sockets[i].domains[j].locations[k].cgi_extensions.size() - 1) std::cout << ", ";
				// }
				// std::cout << "];\n";
				// std::cout << "\t\t\tcgi_path: [";
				// for (size_t l = 0; l < sockets[i].domains[j].locations[k].cgi_paths.size(); ++l) {
				// 	std::cout << sockets[i].domains[j].locations[k].cgi_paths[l];
				// 	if (j < location->cgi_paths.size() - 1) std::cout << ", ";
				// }
				// std::cout << "];\n";
				std::cout << "\t\t\tinterpreters: {\n";
				for (std::map<std::string, std::string>::const_iterator it = sockets[i].domains[j].locations[k].interpreters.begin();
					 it != sockets[i].domains[j].locations[k].interpreters.end(); ++it) {
					std::cout << "\t\t\t\t" + it->first + " " + it->second << ";\n";
				}
				std::cout << "\t\t\t};\n";
				std::cout << "\t\t}\n";
				// if (k < sockets[i].domains[j].locations.size() - 1) std::cout << "\t\tlocation {\n";
			}
			std::cout << "\t}\n";
			// if (j < sockets[i].domains.size() - 1) std::cout << "\tdomain {\n";
		}
		std::cout << "}\n";
	}
	return;
}
