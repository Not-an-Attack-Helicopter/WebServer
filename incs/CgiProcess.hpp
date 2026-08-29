#pragma once
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <ctime>

// Result from running a CGI program
struct CGIResult {
    int status; // optional HTTP Status returned via "Status: 200 OK" header (0 if not provided)
    std::map<std::string, std::string> headers; // parsed headers from CGI stdout
    std::string body;    // body (after headers)
    int         exit_code; // process exit code (-1 if it never started)
    std::string raw_output; // raw stdout (headers + body)
    CGIResult(): status(0), exit_code(-1) {}
};

// Forks + execve's `path`, exposes the pipe ends non-blocking so the caller
// can register them with its own poll()/epoll() and drive I/O only on
// readiness. No internal blocking wait anywhere in this class.
class CgiProcess {
public:
    CgiProcess(const std::string& path,
               const std::vector<std::string>& args,
               const std::map<std::string, std::string>& env,
               const std::string& input,
               const std::string& working_dir = "");
    ~CgiProcess();

    bool  valid() const; // false if pipe()/fork() failed
    pid_t pid()   const;
    int   stdinFd()  const; // -1 once the write end is closed
    int   stdoutFd() const; // -1 once the read end is closed

    bool wantsWrite() const;
    bool wantsRead()  const;
    bool isDone()     const; // both pipe ends closed and child reaped

    void handleWritable(); // one non-blocking write attempt
    void handleReadable(); // drain readable bytes until EAGAIN/EOF/error
    bool tryReap(bool block = false); // waitpid; sets exit code once reaped

    bool isExpired(time_t now) const;
    void forceKill(); // SIGKILL; caller still needs to tryReap()

    CGIResult result() const;

private:
    CgiProcess(const CgiProcess&);
    CgiProcess& operator=(const CgiProcess&);

    pid_t       _pid;
    int         _stdin_fd;
    int         _stdout_fd;
    std::string _input;
    size_t      _input_offset;
    std::string _output;
    bool        _reaped;
    int         _exit_code;
    time_t      _deadline;
};

// Blocking convenience wrapper around CgiProcess for standalone/offline use
// (tests). The live server drives a CgiProcess from its own epoll loop
// instead of calling this.
CGIResult run_cgi(const std::string& path,
                  const std::vector<std::string>& args,
                  const std::map<std::string, std::string>& env,
                  const std::string& input,
                  const std::string& working_dir = "");
