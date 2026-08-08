#include "../includes/cgi_env.hpp"
#include <sstream>
#include <cstdlib>

static std::string to_string_int(int v) {
    std::ostringstream oss; oss << v; return oss.str();
}

std::map<std::string,std::string> build_cgi_env(const HTTPRequest& req,
                                               const ServerConfig& srv,
                                               const LocationConfig& loc,
                                               const std::string& script_filename)
{
    std::map<std::string,std::string> env;

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["REQUEST_METHOD"] = req.getMethod();
    env["SERVER_PROTOCOL"] = req.getVersion();
    env["REQUEST_URI"] = req.getUri();
    env["SCRIPT_FILENAME"] = script_filename;
    env["SCRIPT_NAME"] = script_filename; // caller can adjust
    env["QUERY_STRING"] = req.getQuery();
    env["DOCUMENT_ROOT"] = srv.server_names.empty() ? "" : srv.server_names[0];
    env["SERVER_NAME"] = srv.server_names.empty() ? srv.host : srv.server_names[0];
    env["SERVER_PORT"] = to_string_int(srv.port);
    env["CONTENT_LENGTH"] = req.getHeader("content-length");
    env["CONTENT_TYPE"] = req.getHeader("content-type");

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
