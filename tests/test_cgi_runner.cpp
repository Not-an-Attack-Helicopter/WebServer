#include <iostream>
#include <vector>
#include <map>
#include "../includes/cgi.hpp"
#include "../includes/cgi_env.hpp"
#include "../includes/HTTPRequest.hpp"
#include "../includes/types.hpp"

static void ASSERT(bool cond, const std::string& msg) {
    if (cond) std::cout << "[PASS] " << msg << "\n";
    else std::cout << "[FAIL] " << msg << "\n";
}

int main() {
    std::vector<std::string> args;
    args.push_back("sh");
    args.push_back("-c");
    args.push_back("printf \"Status: 201\\r\\nContent-Type: text/plain\\r\\n\\r\\nHello CGI\\n\"");

    std::map<std::string,std::string> env;
    env["REQUEST_METHOD"] = "GET";

    CGIResult res = run_cgi("/bin/sh", args, env, "");

    ASSERT(res.exit_code != -1, "CGI executed (exit_code set)");
    ASSERT(res.headers.find("content-type") != res.headers.end(), "Content-Type header parsed");
    ASSERT(res.headers.at("content-type") == "text/plain", "Content-Type value correct");
    ASSERT(res.status == 201, "Status header parsed as 201");
    ASSERT(res.body.find("Hello CGI") != std::string::npos, "Body contains Hello CGI");

    std::cout << "exit_code=" << res.exit_code << "\n";
    // env builder test
    HTTPRequest req;
    std::string raw = "POST /cgi-bin/script.py?x=1 HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nhello";
    req.parse(raw);
    ServerConfig srv; srv.host = "127.0.0.1"; srv.port = 8080; srv.server_names.push_back("localhost");
    LocationConfig loc; loc.path = "/cgi-bin"; loc.root = ".";
    std::map<std::string,std::string> env2 = build_cgi_env(req, srv, loc, "./cgi-bin/script.py");
    ASSERT(env2["REQUEST_METHOD"] == "POST", "env REQUEST_METHOD set");
    ASSERT(env2["QUERY_STRING"] == "x=1", "env QUERY_STRING set");

    return 0;
}
