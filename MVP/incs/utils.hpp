#pragma once

// #include "webserver.hpp"
#include "types.hpp"
#include <sys/epoll.h>
#include <string>
#include <vector>

// DEBUG
void			warnHighEventLoad(int nfds, int max_capacity);
void			dumpEvents(int nfds, epoll_event* events);
void			check_tcp_drops(void);
// DEBUG
bool			valid_ip(const std::string& ip);
bool			valid_port(const std::string& port_str);
bool			is_address_already_used(const std::vector<Config>& config, const std::string& host, int port);
bool			valid_config_line(const std::string& line);
bool			is_valid_method(const std::string& method);
bool			is_valid_body_size(const int size);
bool			is_valid_error_code(const int code);

// std::string		i2a(short input);
// std::string		i2a(const short input);
std::string		i2a(unsigned short input);
// std::string		i2a(const unsigned short input);
std::string		i2a(int input);
// std::string		i2a(const int input);
// std::string		i2a(unsigned int input);
// std::string		i2a(const unsigned int input);
std::string		i2a(long input);
// std::string		i2a(const long input);
std::string		i2a(unsigned long input);
// std::string		i2a(const unsigned long input);
std::string		i2a(double input);
// std::string		i2a(const double input);

std::string		trim(const std::string& str);
std::string		get_content_type(const std::string& path);

void			print_conf(const std::vector<Config>& config);
