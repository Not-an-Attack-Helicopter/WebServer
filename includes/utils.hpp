#pragma once

#include "webserver.hpp"

std::string trim(const std::string& str);
bool valid_ip(const std::string& ip);
bool valid_port(const std::string& port_str);
bool is_address_already_used(const Config& config, const std::string& host, int port);
bool valid_config_line(const std::string& line);
void print_conf(const Config& config);
bool is_valid_method(const std::string& method);
bool is_valid_body_size(const int size);
bool is_valid_error_code(const int code);
std::string get_content_type(const std::string& path);
