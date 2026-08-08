#pragma once
#include <string>
#include <vector>
#include <map>

// Result from running a CGI program
struct CGIResult {
    int status; // optional HTTP Status returned via "Status: 200 OK" header (0 if not provided)
    std::map<std::string, std::string> headers; // parsed headers from CGI stdout
    std::string body;    // body (after headers)
    int         exit_code; // process exit code (-1 if it never started)
    std::string raw_output; // raw stdout (headers + body)
    CGIResult(): status(0), exit_code(-1) {}
};

// Runs `path` (executable) with `args` (argv vector), `env` (environment map),
// feeds `input` to the program's stdin, waits for completion, parses CGI headers
// from stdout and returns `CGIResult` containing headers/body and exit code.
// `working_dir` (optional): if non-empty, the child will `chdir()` to it before execve.
CGIResult run_cgi(const std::string& path,
                  const std::vector<std::string>& args,
                  const std::map<std::string, std::string>& env,
                  const std::string& input,
                  const std::string& working_dir = "");
