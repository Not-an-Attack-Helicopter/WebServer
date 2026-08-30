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
#include <fcntl.h>
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
	for (std::size_t i = 0; i < soc.domains.size(); ++i) {
		for (std::size_t j = 0; j < soc.domains[i].names.size(); ++j) {
			log.info(soc.domains[i].names[j]);
		}
		log.info(soc.domains[i].root);
		for (std::size_t j = 0; j < soc.domains[i].index_files.size(); ++j) {
			log.info(soc.domains[i].index_files[j]);
		}
		for (std::size_t j = 0; j < soc.domains[i].locations.size(); ++j) {
			log.info(soc.domains[i].locations[j].path);
			log.info(soc.domains[i].locations[j].root);
			for (std::size_t k = 0; k < soc.domains[i].locations[j].index_files.size(); ++k) {
				log.info(soc.domains[i].locations[j].index_files[k]);
			}
		}
	}
}

void dumpRequest(const HTTPRequest* request) {

	std::string state;
	switch(request->parsing.state) {
	case HTTPRequest::READING_REQUEST_LINE: state = "reading request line"; break;
	case HTTPRequest::READING_HEADERS: state = "reading headers"; break;
	case HTTPRequest::RESOLVING_ROUTE: state = "resolving route"; break;
	case HTTPRequest::READING_BODY: state = "reading body"; break;
	case HTTPRequest::CGI_PROCESSING: state = "processing"; break;
	case HTTPRequest::COMPLETE: state = "complete"; break;
	case HTTPRequest::ERROR: state = "error"; break;
	}
	log.debug("State:\t\t" + state + " (" + i2a(request->parsing.state) + ")");

	switch (request->getMethod()) {
	case GET: log.debug("Method:\t\tGET"); break;
	case HEAD: log.debug("Method:\t\tHEAD"); break;
	case DELETE: log.debug("Method:\t\tDELETE"); break;
	case POST: log.debug("Method:\t\tPOST"); break;
	case PUT: log.debug("Method:\t\tPUT"); break;
	case METHOD_COUNT: log.debug("Method:\t\tN/A"); break;
	}

	log.debug("Path:\t\t" + request->getPath());
	log.debug("Query:\t\t" + request->getQuery());
	log.debug("Version:\t" + request->getVersion());

	// Print all headers
	// std::map<std::string, std::string>::iterator it = request->getHeaders().begin();
	// while (it != request->getHeaders().end()) {
	// 	log.debug(it->first + ":\t" + it->second);
	// 	++it;
	// }

	// Print select headers
	const std::string* host = request->getHeader("host");
	if (host != NULL)
		log.debug("Host:\t\t" + *host);
	const std::string* user_agent = request->getHeader("user-agent");
	if (user_agent != NULL)
		log.debug("User-Agent:\t" + *user_agent);
	const std::string* accept = request->getHeader("accept");
	if (accept != NULL)
		log.debug("Accept:\t\t" + *accept);
	const std::string* connection = request->getHeader("connection");
	if (connection != NULL)
		log.debug("Connection:\t" + *connection);
	const std::string* type = request->getHeader("content-type");
	if (type != NULL)
		log.debug("Content-Type:\t" + *type);
	const std::string* disposition = request->getHeader("content-disposition");
	if (disposition != NULL)
		log.debug("Content-Type:\t" + *disposition);
	const std::string* content_length = request->getHeader("content-length");
	if (content_length != NULL)
		log.debug("Content-Length:\t" + *content_length);
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

std::size_t stringToSize(const std::string& str) {

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

	return static_cast<std::size_t>(tmp);

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
	std::size_t first = str.find_first_not_of(" \t\r\n");
	std::size_t last = str.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

std::string unquote(const std::string& str) {
	if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"')
		return str.substr(1, str.size() - 2);
	return str;
}

std::string randomHexString(std::size_t width) {

	static const char hex[] = "0123456789abcdef";

	unsigned char* bytes = new unsigned char[width];

	try {
		std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);

		if (!urandom) {
			throw std::runtime_error("cannot open /dev/urandom");
		}

		urandom.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(width));

		if (urandom.gcount() != static_cast<std::streamsize>(width)) {
			throw std::runtime_error("cannot read /dev/urandom");
		}

		std::string result;
		result.reserve(width * 2);

		for (std::size_t i = 0; i < width; ++i) {
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

	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) return false;
	return S_ISREG(sb.st_mode);

}

bool isDirectory(const std::string& path) {

	struct stat sb;
	if (stat(path.c_str(), &sb) != 0) return false;
	return S_ISDIR(sb.st_mode);

}

void createFile(HTTPRequest& request) {

	const Config::Location& location = *request.resolved.location;

	log.info("creating file...!");
	std::string directory;
	if (!location.root.empty()) {
		directory = location.root;
	} else {
		directory = location.alias;
	}

	std::string suffix;
	int file_descriptor;
	unsigned short count = 0;
	std::time_t timestamp = std::time(NULL);
	do {
		try {
			suffix = randomHexString(5);
		} catch (std::exception& e) {
			log.warn("random hex string generator: " + std::string(e.what()));
			std::stringstream oss;
			oss << timestamp + std::time(NULL);
			suffix = oss.str();
		}
		std::string unique_id = i2a(timestamp) + "-" + suffix;
		std::string file_path = directory + location.upload_dir + "/.upload_" + unique_id + ".part";
		file_descriptor = open(file_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
		log.debug("file_fd: " + i2a(file_descriptor) + "\tfile_path: " + file_path);
		if (request.is_multipart) {
			request.body.parts.back().file = file_descriptor;
			request.body.parts.back().path = file_path;
		} else {
			request.body.file = file_descriptor;
			request.body.path = file_path;
		}
	} while (file_descriptor == -1 && errno == EEXIST && ++count < 11);

	request.parsing.state = HTTPRequest::READING_BODY;
	return;

}

static std::string extraxtExtension(const std::string& filename) {

	std::size_t slash_pos = filename.find_last_of("/\\");
	std::size_t from_pos = (slash_pos == std::string::npos) ? 0 : slash_pos + 1;
	std::size_t dot_pos = filename.find('.', from_pos);

	if (dot_pos == std::string::npos) return "";

	return filename.substr(dot_pos);

}

void promoteFile(HTTPRequest& request) {

	std::string old_path;
	std::string extension;
	if (request.is_multipart) {
		close(request.body.parts.back().file);
		old_path = request.body.parts.back().path;
		extension = extraxtExtension(request.body.parts.back().filename);
	} else {
		close(request.body.file);
		old_path = request.body.path;
		extension = extraxtExtension(request.body.filename);
	}

	std::string suffix;
	std::time_t timestamp = std::time(NULL);
	try {
		suffix = randomHexString(7);
	} catch (std::exception& e) {
		log.warn("random hex string generator: " + std::string(e.what()));
		std::stringstream oss;
		oss << timestamp + std::time(NULL);
		suffix = oss.str();
	}
	std::string new_path;
	if (request.resolved.method == POST) {
		const std::string& path = request.resolved.path;
		const Config::Location& location = *request.resolved.location;
		new_path = path + location.upload_dir + "/upload-" + suffix + extension;
	} else if (request.resolved.method == PUT) {
		new_path = request.resolved.path;
	} else {
		new_path = old_path + "." + suffix + extension;
	}
	log.debug("new path: " + new_path);

	if (std::rename(old_path.c_str(), new_path.c_str()) != 0) {
		log.warn("dispatch error: " + std::string(strerror(errno)));
		return;
	}

	if (request.is_multipart) {
		request.body.parts.back().path = new_path;
	} else {
		request.body.path = new_path;
	}

	return;

}

void dumpConfigs(const std::vector<Config::Socket>& sockets) {
	for (std::size_t i = 0; i < sockets.size(); ++i) {
		std::cout << "socket {\n";
		std::cout << "\thost: " << sockets[i].address << ";\n";
		std::cout << "\tport: " << sockets[i].port << ";\n";
		std::cout << "\tclient_max_body_size: " << sockets[i].client_max_body_size << ";\n";
		for (std::size_t j = 0; j < sockets[i].domains.size(); ++j) {
			std::cout << "\tdomain {\n";
			std::cout << "\t\tnames: [";
			for (std::size_t k = 0; k < sockets[i].domains[j].names.size(); ++k) {
				std::cout << sockets[i].domains[j].names[k];
				if (k < sockets[i].domains[j].names.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "\t\troot: " << sockets[i].domains[j].root << ";\n";
			std::cout << "\t\tindex_files: [";
			for (std::size_t k = 0; k < sockets[i].domains[j].index_files.size(); ++k) {
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
			for (std::size_t k = 0; k < sockets[i].domains[j].locations.size(); ++k) {
				std::cout << "\t\tlocation {\n";
				std::cout << "\t\t\tpath " << sockets[i].domains[j].locations[k].path << ";\n";
				std::cout << "\t\t\troot: " << sockets[i].domains[j].locations[k].root << ";\n";
				std::cout << "\t\t\talias: " << sockets[i].domains[j].locations[k].alias << ";\n";
				std::cout << "\t\t\tredirect: " << sockets[i].domains[j].locations[k].redirect << ";\n";
				std::cout << "\t\t\tmethods: [";
				for (std::size_t l = 0; l < sockets[i].domains[j].locations[k].methods.size(); ++l) {
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
				for (std::size_t l = 0; l < sockets[i].domains[j].locations[k].index_files.size(); ++l) {
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
				std::cout << "\t\t\tinterpreters: {\n";
				for (std::map<std::string, std::string>::const_iterator it = sockets[i].domains[j].locations[k].interpreters.begin();
					 it != sockets[i].domains[j].locations[k].interpreters.end(); ++it) {
					std::cout << "\t\t\t\t" + it->first + " " + it->second << ";\n";
				}
				std::cout << "\t\t\t};\n";
				std::cout << "\t\t}\n";
			}
			std::cout << "\t}\n";
		}
		std::cout << "}\n";
	}
	return;
}

/*
 * ================================================================
 * ASCII helpers
 * ================================================================
 */

static char tolowerASCII(char c) {

	unsigned char uc = static_cast<unsigned char>(c);

	if (uc >= static_cast<unsigned char>('A') &&
		uc <= static_cast<unsigned char>('Z')) {

		uc = static_cast<unsigned char>(
			uc + ('a' - 'A'));
	}

	return static_cast<char>(uc);

}

std::string tolowerASCII(const std::string& s) {

	std::string result(s);

	std::size_t i;

	for (i = 0; i < result.size(); ++i) {
		result[i] = tolowerASCII(result[i]);
	}

	return result;

}

int hexDigitValue(char c) {

	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;

}

/*
* RFC 9110:
*
* tchar = "!" / "#" / "$" / "%" / "&" / "'"
*       / "*" / "+" / "-" / "." / "^" / "_"
*       / "`" / "|" / "~" / DIGIT / ALPHA
*/
bool isTChar(char c) {
	unsigned char uc = static_cast<unsigned char>(c);
	if ((uc >= 'A' && uc <= 'Z') ||
		(uc >= 'a' && uc <= 'z') ||
		(uc >= '0' && uc <= '9')) {
		return true;
	}

	switch (uc) {

	case '!':
	case '#':
	case '$':
	case '%':
	case '&':
	case '\'':
	case '*':
	case '+':
	case '-':
	case '.':
	case '^':
	case '_':
	case '`':
	case '|':
	case '~':
		return true;

	default:
		return false;
	}

}

bool isHexDigit(char c) {
	return (c >= '0' && c <= '9') ||
	(c >= 'A' && c <= 'F') ||
	(c >= 'a' && c <= 'f');
}

bool equalCI(const std::string& a,
			 const std::string& b) {
	if (a.size() != b.size()) {
		return false;
	}

	std::size_t i;

	for (i = 0; i < a.size(); ++i) {

		if (tolowerASCII(a[i]) !=
			tolowerASCII(b[i])) {

			return false;
		}
	}

	return true;
}
