/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISetUp.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gpochon, bstorck <marvin@42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 08:41:59 by gpochon           #+#    #+#             */
/*   Updated: 2026/09/13 08:42:01 by gpochon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/CGISetUp.hpp"
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"

#include <arpa/inet.h>
#include <set>

// static std::string to_string_int(int v) {
//     std::ostringstream oss; oss << v; return oss.str();
// }

// dotted-decimal string from a sockaddr_in's binary address, e.g. "127.0.0.1"
static inline std::string addr2string(const sockaddr_in& addr) {
    char buf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
    return std::string(buf);
}

static inline std::string method2string(const Method& method) {
    switch (method) {
        case GET:    return "GET";
        case HEAD:   return "HEAD";
        case DELETE: return "DELETE";
        case POST:   return "POST";
        case PUT:    return "PUT";
        default:     return "";
    }
}

// Build a map of CGI environment variables from the request and server/location config.
static std::vector<std::string> buildCGIenv(const HTTPRequest& req) {

	std::map<std::string,std::string> env_map;
	std::set<std::string> ignore;

	env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
	env_map["REDIRECT_STATUS"] = "1";
	env_map["REQUEST_METHOD"] = method2string(req.getMethod());
	env_map["SERVER_PROTOCOL"] = req.getVersion();
	env_map["REQUEST_URI"] = req.getQuery().empty() ? req.getPath() : req.getPath() + "?" + req.getQuery();
	env_map["SCRIPT_FILENAME"] = req.resolved.filepath;
	env_map["SCRIPT_NAME"] = req.cgi.script_name; // decoded, no path_info, no root -- set in resolveRoute()
	env_map["QUERY_STRING"] = req.getQuery();
	env_map["DOCUMENT_ROOT"] = req.resolved.domain->root;
	env_map["SERVER_NAME"] = req.resolved.domain->names[0]; // extractDomainNames() throws on empty, always non-empty here
	env_map["SERVER_PORT"] = i2a(ntohs(req.cgi.server_socket.sin_port)); // sin_port is network byte order, need to ntohs()
	env_map["PATH_INFO"] = req.cgi.path_info;
	env_map["PATH_TRANSLATED"] = req.cgi.path_translated;
	env_map["REMOTE_ADDR"] = addr2string(req.cgi.remote_socket);
	env_map["REMOTE_PORT"] = i2a(ntohs(req.cgi.remote_socket.sin_port));
	env_map["SERVER_ADDR"] = addr2string(req.cgi.server_socket);

	const std::string* length = req.getHeader("content-length");
	if (length != NULL) {
		env_map["CONTENT_LENGTH"] = *length;
		ignore.insert("content-length");
	}
	const std::string* type = req.getHeader("content-type");
	if (type != NULL) {
		env_map["CONTENT_TYPE"] = *type;
		ignore.insert("content-type");
	}

	// Copy HTTP_... headers
	std::string tmp;
	std::string key;
	const std::map<std::string,std::string>& headers = req.getHeaders();
	for (std::map<std::string,std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it) {
		tmp = it->first;

		// content-type/length don't get the HTTP_ prefix, skipping them early to avoid building unnecessary string
		if (ignore.count(tmp)) continue;

		// transform header name to CGI HTTP_ form
		key.clear();
		key.reserve(5 + tmp.size());  // "HTTP_" + key
		key.append("HTTP_");
		for (size_t i = 0; i < tmp.size(); ++i) {
			char c = tmp[i];
			key.push_back(c == '-' ? '_' : static_cast<char>(toupper(c)));
		}

		env_map[key] = it->second;
	}

	// Convert string pairs to holisic strings and store them in a vector
	std::vector<std::string> env_vector;
	env_vector.reserve(env_map.size());
	for (std::map<std::string, std::string>::const_iterator it = env_map.begin();
		it != env_map.end(); ++it) {
		env_vector.push_back(it->first + "=" + it->second);
	}

	return env_vector;
}

StatusCode setUpCGI(Client& client) {

	HTTPRequest& request = client.getCurrentRequest();

	std::string cgi_input; // Delete this

	// argv for execve
	std::vector<std::string> cgi_args;
	cgi_args.push_back(request.cgi.binary_path);
	cgi_args.push_back(request.resolved.filepath);
	if (cgi_args.empty()) {
		cgi_args.push_back(request.cgi.binary_path);
	}
	std::string working_dir = request.resolved.filepath.substr(0, request.resolved.filepath.find_last_of('/'));

	std::vector<std::string> env = buildCGIenv(request);

	if (client.cgi_process != NULL) {
		delete client.cgi_process;
		client.cgi_process = NULL;
	}
	client.cgi_process = new CGIProcess(request.cgi.binary_path, cgi_args, env, working_dir);
	// CGIProcess* cgi_process = new CGIProcess(request.cgi.binary_path, cgi_args, env, working_dir);

	if (!client.cgi_process->valid()) {
		log.error("cgi: failed to open pipes for " + request.cgi.binary_path);
		return INTERNAL_SERVER_ERROR;
	}

	if (!client.cgi_process->spawn()) {
		log.error("cgi: failed to spawn " + request.cgi.binary_path);
		return INTERNAL_SERVER_ERROR;
	}

	// client.process_queue.push_back(cgi_process);

	// epoll registration is still ahead, server side
	return NO_STATUS;
}
