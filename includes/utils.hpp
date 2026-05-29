#pragma once

#include "webserver.hpp"

std::string trim(const std::string& str);
bool valid_ip(const std::string& ip);
bool valid_port(const std::string& port_str);
bool is_address_already_used(const Config& config, const std::string& host, int port);
bool valid_config_line(const std::string& line);
void print_conf(const Config& config);
