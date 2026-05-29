#include "webserver.hpp"

LocationConfig  parse_location_block(const std::vector<std::string>& tokens, size_t& i);
ServerConfig    parse_server_block(const std::vector<std::string>& tokens, size_t& i, const Config& config);
Config          parse_config_file(const std::string& config_file);
bool            valid_CGI(const std::string& path);
bool            valid_CGI_ext(const std::string& ext);
bool            valid_conf_ext(const std::string& filename);