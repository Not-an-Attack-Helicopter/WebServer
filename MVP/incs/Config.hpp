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

struct LocationConfig {
	std::string						path;					//< Location path (e.g., "/api", "/cgi-bin")
	std::vector<std::string>		methods;				//< Allowed HTTP methods (GET, POST, DELETE)
	bool							autoindex;				//< Enable/disable directory listing
	std::string						index;					//< Default index file for this location
	std::string						root;					//< Root directory for this location
	std::string						redirect;				//< Redirect URL (return directive)
	std::string						upload_dir;				//< Directory for file uploads
	std::vector<std::string>		cgi_extensions;			//< CGI file extensions (.py, .sh, etc.)
	std::vector<std::string>		cgi_paths;				//< Corresponding CGI interpreter paths
};

struct Config {
	in_port_t						port;					//< Listen port (1–65535)
	std::string						host;					//< Bind host IP address
	std::vector<std::string>		server_names;			//< List of server names (SNI)
	std::string						root;					//< Root directory for server
	std::string						index;					//< Default index file for server
	size_t							client_max_body_size;	//< Maximum request body size in bytes
	std::map<int, std::string>		error_pages;			//< Error code -> error page file mapping
	std::vector<LocationConfig>		locations;				//< Location blocks within this server
};

#endif
