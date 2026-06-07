#pragma once
#include <string>
#include <vector>
#include <map>

// What we get back after running a CGI program
struct CGIResult {
    std::string output;    // what the program printed to stdout
    int         exit_code; // how it exited (-1 if it never started)
};

// Runs "path" as a CGI program, feeds it "input" on stdin
// and returns what it printed + how it exited.
CGIResult run_cgi(const std::string& path, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, const std::string& input);
