#pragma once

// #include "webserver.hpp"
#include "types.hpp"

std::string						i2a(int input);
LocationConfig					parse_location_block(const std::vector<std::string>& tokens, size_t& i);
ServerConfig					parse_server_block(const std::vector<std::string>& tokens, size_t& i, const std::vector<ServerConfig>& config);
std::vector<ServerConfig>		parse_config_file(const std::string& config_file);
bool							valid_CGI(const std::string& path);
bool							valid_CGI_ext(const std::string& ext);
bool							valid_conf_ext(const std::string& filename);
