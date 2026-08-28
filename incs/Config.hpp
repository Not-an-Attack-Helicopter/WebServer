/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/04 15:01:02 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/04 15:01:04 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

// #include "Method.hpp"
#include <netinet/in.h>
// #include <cstddef>
#include <string>
#include <vector>
#include <map>

#define configs Config::instance()

enum Sink {
	NONE,
	HEAP,
	DISK
};

enum Method {
	GET,
	HEAD,
	DELETE,
	POST,
	PUT,
	METHOD_COUNT
};

class Config {

public:

	static const size_t						SERVER_MAX_BODY_SIZE = std::size_t(16)*1024*1024*1024; // 16 GiB

	struct Location {
		std::string								path;					// Location path (e.g., "/api", "/cgi-bin")
		std::string								root;					// Root directory for this location
		std::string								alias;					// File system directory for this location
		std::string								redirect;				// Redirect URL (return directive)
		// std::vector<std::string>				methods;				// Allowed HTTP methods (GET/POST/DELETE)
		std::vector<Method>						methods;				// Allowed HTTP methods (GET/POST/DELETE)
		bool									autoindex;				// Enable/disable directory listing
		std::vector<std::string>				index_files;			// Default index files for this location
		std::map<int, std::string>				error_pages;			// Error code -> error page file mapping
		std::string								upload_dir;				// Directory for file uploads
		size_t									client_max_body_size;	// Maximum request body size in bytes
		// std::vector<std::string>				cgi_extensions;			// CGI file extensions (.py, .sh, etc.)
		// std::vector<std::string>				cgi_paths;				// Corresponding CGI interpreter paths
		std::map<std::string, std::string>		interpreters;			// CGI file ext -> CGI interpreter path

		Location(void)
			:	path(""),
				root(""),
				alias(""),
				redirect(""),
				autoindex(false),
				upload_dir(""),
				client_max_body_size(SERVER_MAX_BODY_SIZE) {
			methods.clear();
			index_files.clear();
			error_pages.clear();
			interpreters.clear();
		}

	};

	struct Domain {
		// std::string								name;					// Hostname/virtual host (e.g., "example.com")
		std::vector<std::string>				names;					// Hostnames/virtual hosts (e.g., "example.com")
		std::string								root;					// Root directory for server
		std::vector<std::string>				index_files;			// Default index file for server
		std::map<int, std::string>				error_pages;			// Error code -> error page file mapping
		size_t									client_max_body_size;	// Maximum request body size in bytes
		std::vector<Location>					locations;				// Location blocks within this server

		Domain(void)
			:	root(""),
				client_max_body_size(SERVER_MAX_BODY_SIZE) {
			names.clear();
			index_files.clear();
			error_pages.clear();
			locations.clear();
		}

	};

	struct Socket {

		in_port_t								port;					// Listen port (1–65535)
		std::string								address;				// Bind host IP address
		size_t									client_max_body_size;	// Maximum request body size in bytes
		std::vector<Domain>						domains;				// Virtual hosts

		Socket(void)
			:	port(8080),
				address(""),
				client_max_body_size(SERVER_MAX_BODY_SIZE) {
			domains.clear();
		}

	};

	static Config&								instance(void);

	const std::vector<Socket>&					get(void) const;

	const Socket& 								get(size_t index) const;

	size_t										size(void) const;

	void										pushConfig(Socket socket);

	// void										validateRedirectChains(void);

private:

	Config(void);
	Config(const Config& other);
	Config& operator = (const Config& other);
	~Config(void);

	// static const unsigned short					MAX_REDIRECTS = 5;

	std::vector<Socket>							_configs;

};

#endif
