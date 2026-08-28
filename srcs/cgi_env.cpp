#include "cgi_env.hpp"
#include "templates.hpp"
#include <cctype>

std::map<std::string, std::string> build_cgi_env(const HTTPRequest& req,
												 const Config::Domain& domain,
												 const Config::Location& loc,
												 const std::string& script_filename)
{
	static_cast<void>(loc);

	static const std::string method_names[static_cast<int>(METHOD_COUNT)] = {
		"GET", "HEAD", "DELETE", "POST", "PUT"
	};

	std::map<std::string, std::string> env;

	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	env["REQUEST_METHOD"] = method_names[req.getMethod()];
	env["SERVER_PROTOCOL"] = req.getVersion();
	env["REQUEST_URI"] = req.getPath();
	env["SCRIPT_FILENAME"] = script_filename;
	env["SCRIPT_NAME"] = req.getPath();
	env["PATH_INFO"] = req.getPath();
	env["QUERY_STRING"] = req.getQuery();
	env["REDIRECT_STATUS"] = "200"; // required by php-cgi

	if (!domain.names.empty()) {
		env["SERVER_NAME"] = domain.names[0];
	} else {
		const std::string* host = req.getHeader("host");
		if (host != NULL) env["SERVER_NAME"] = *host;
	}

	// Port comes from the Host header (after ':')
	const std::string* host = req.getHeader("host");
	if (host != NULL) {
		size_t colon = host->find(':');
		if (colon != std::string::npos) env["SERVER_PORT"] = host->substr(colon + 1);
	}

	const std::string* content_length = req.getHeader("content-length");
	if (content_length != NULL) env["CONTENT_LENGTH"] = *content_length;
	const std::string* content_type = req.getHeader("content-type");
	if (content_type != NULL) env["CONTENT_TYPE"] = *content_type;

	// Copy remaining headers as HTTP_... variables
	const std::map<std::string, std::string>& headers = req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::string h = "HTTP_";
		for (size_t i = 0; i < it->first.size(); ++i) {
			char c = it->first[i];
			if (c == '-') h.push_back('_');
			else h.push_back((char)std::toupper((unsigned char)c));
		}
		if (h == "HTTP_CONTENT_TYPE" || h == "HTTP_CONTENT_LENGTH") continue;
		env[h] = it->second;
	}

	return env;
}
