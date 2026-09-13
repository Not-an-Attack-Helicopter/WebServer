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
#include "../incs/Logger.hpp"
#include <arpa/inet.h>

static std::string to_string_int(int v) {
    std::ostringstream oss; oss << v; return oss.str();
}

// dotted-decimal string from a sockaddr_in's binary address, e.g. "127.0.0.1"
static std::string addr_to_string(const sockaddr_in& addr) {
    char buf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
    return std::string(buf);
}

static std::string method_to_string(const Method& method) {
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
static std::map<std::string,std::string> build_cgi_env(const HTTPRequest& req,
													   const Config::Domain& domain,
													   const Config::Location& loc,
													   const std::string& script_filename)
{
    std::map<std::string,std::string> env;
    (void)loc;

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["REQUEST_METHOD"] = method_to_string(req.getMethod());
    env["SERVER_PROTOCOL"] = req.getVersion();
    env["REQUEST_URI"] = req.getQuery().empty() ? req.getPath() : req.getPath() + "?" + req.getQuery();
    env["SCRIPT_FILENAME"] = script_filename;
    env["SCRIPT_NAME"] = req.cgi.script_name; // decoded, no path_info, no root -- set in resolveRoute()
    env["QUERY_STRING"] = req.getQuery();
    env["DOCUMENT_ROOT"] = domain.root;
    env["SERVER_NAME"] = domain.names[0]; // extractDomainNames() throws on empty, always non-empty here
    env["SERVER_PORT"] = to_string_int(req.cgi.server_socket.sin_port);

    if (!req.cgi.path_info.empty()) {
        env["PATH_INFO"] = req.cgi.path_info;
        env["PATH_TRANSLATED"] = req.resolved.filepath;
    }

    // sin_port is network byte order, ntohs() before treating it as a number
    env["REMOTE_ADDR"] = addr_to_string(req.cgi.remote_socket);
    env["REMOTE_PORT"] = to_string_int(ntohs(req.cgi.remote_socket.sin_port));
    env["SERVER_ADDR"] = addr_to_string(req.cgi.server_socket);

    const std::string* content_length = req.getHeader("content-length");
    if (content_length != NULL) env["CONTENT_LENGTH"] = *content_length;
    const std::string* content_type = req.getHeader("content-type");
    if (content_type != NULL) env["CONTENT_TYPE"] = *content_type;

    // Copy HTTP_... headers
    const std::map<std::string,std::string>& headers = req.getHeaders();
    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = it->first;
        std::string val = it->second;
        // transform header name to CGI HTTP_ form
        std::string h = "HTTP_";
        for (size_t i = 0; i < key.size(); ++i) {
            char c = key[i];
            if (c == '-') h.push_back('_');
            else h.push_back((char)toupper(c));
        }
        // skip content-type/length as they are separate
        if (h == "HTTP_CONTENT_TYPE" || h == "HTTP_CONTENT_LENGTH") continue;
        env[h] = val;
    }

    return env;
}

StatusCode setUpCGI(Client& client) {

	HTTPRequest& request = client.getCurrentRequest();

	std::string cgi_input; // Delete this

	// argv for execve
	std::vector<std::string> cgi_args;
	cgi_args.push_back(request.cgi.binary_path);
	cgi_args.push_back(request.resolved.filepath);
	std::string working_dir = request.resolved.filepath.substr(0, request.resolved.filepath.find_last_of('/'));

	std::map<std::string, std::string> env = build_cgi_env(request,
														   *request.resolved.domain,
														   *request.resolved.location,
														   request.resolved.filepath);

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
