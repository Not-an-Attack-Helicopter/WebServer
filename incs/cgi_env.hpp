#ifndef CGI_ENV_HPP
#define CGI_ENV_HPP

#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "Config.hpp"

// Build a map of CGI environment variables from the request and domain/location config.
std::map<std::string, std::string> build_cgi_env(const HTTPRequest& req,
												 const Config::Domain& domain,
												 const Config::Location& loc,
												 const std::string& script_filename);

#endif
