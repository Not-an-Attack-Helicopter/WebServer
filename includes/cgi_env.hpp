#pragma once
#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "types.hpp"

// Build a map of CGI environment variables from the request and server/location config.
std::map<std::string,std::string> build_cgi_env(const HTTPRequest& req,
                                               const ServerConfig& srv,
                                               const LocationConfig& loc,
                                               const std::string& script_filename);
