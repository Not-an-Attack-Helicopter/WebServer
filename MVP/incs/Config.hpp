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

#include <netinet/in.h>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

struct Location {

	std::string								path;			// Location path (e.g., "/api", "/cgi-bin")
	std::string								root;			// Root directory for this location
	std::string								redirect;		// Redirect URL (return directive)
	std::vector<std::string>				methods;		// Allowed HTTP methods (GET/POST/DELETE)
	bool									autoindex;		// Enable/disable directory listing
	std::vector<std::string>				index_files;	// Default index files for this location
	std::string								upload_dir;		// Directory for file uploads
	// std::vector<std::string>				cgi_extensions;	// CGI file extensions (.py, .sh, etc.)
	// std::vector<std::string>				cgi_paths;		// Corresponding CGI interpreter paths
	std::map<std::string, std::string>		interpreters;	// CGI file ext -> CGI interpreter path
	std::map<int, std::string>				error_pages;	// Error code -> error page file mapping

};

struct Config {

	in_port_t						port;					// Listen port (1–65535)
	std::string						host;					// Bind host IP address
	std::vector<std::string>		server_names;			// List of server names
	std::string						root;					// Root directory for server
	std::vector<std::string>		index_files;			// Default index file for server
	std::map<int, std::string>		error_pages;			// Error code -> error page file mapping
	size_t							client_max_body_size;	// Maximum request body size in bytes
	std::vector<Location>			locations;				// Location blocks within this server

};

#endif
