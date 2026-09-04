#include "../incs/CgiEnv.hpp"
#include <sstream>
#include <cstdlib>
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

std::map<std::string,std::string> build_cgi_env(const HTTPRequest& req,
                                               const Config::Socket& socket,
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
    env["SCRIPT_NAME"] = script_filename; // caller can adjust
    env["QUERY_STRING"] = req.getQuery();
    env["DOCUMENT_ROOT"] = domain.root;
    env["SERVER_NAME"] = domain.names.empty() ? socket.address : domain.names[0];
    env["SERVER_PORT"] = to_string_int(socket.port);

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
