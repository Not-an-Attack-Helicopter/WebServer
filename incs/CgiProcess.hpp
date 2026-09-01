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

// Opens the pipes in the constructor (cheap, easily rolled back), then waits
// for spawn() to actually fork()+execve() `path` once the caller has
// registered the pipe fds with its own poll()/epoll(). Exposes the pipe ends
// non-blocking so the caller can drive I/O only on readiness. No internal
// blocking wait anywhere in this class.
class CgiProcess {
public:
    CgiProcess(const std::string& path,
               const std::vector<std::string>& args,
               const std::map<std::string, std::string>& env,
               const std::string& input,
               const std::string& working_dir = "");
    ~CgiProcess();

    bool  valid() const; // false if pipe() failed
    bool  spawn();        // fork()+execve()'s using the already-open pipes; false on failure
    pid_t pid()   const;
    int   stdinFd()  const; // -1 once the write end is closed
    int   stdoutFd() const; // -1 once the read end is closed

    // for a caller doing its own buffered write()/read() against
    // stdinFd()/stdoutFd() (e.g. CgiHandler) instead of handleWritable()/
    // handleReadable() -- closes the fd and updates it to -1, same as those
    // do internally once done, so wantsWrite()/wantsRead()/isDone() and the
    // destructor stay correct either way.
    void closeStdin();
    void closeStdout();

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

    // stored for spawn() (next step), which forks+execve's using these
    std::string                        _path;
    std::vector<std::string>           _args;
    std::map<std::string, std::string> _env;
    std::string                        _working_dir;

    // raw pipe ends opened by the constructor; consumed by spawn()
    int         _in_pipe[2];  // [0] read end (child stdin), [1] write end (we write the body here)
    int         _out_pipe[2]; // [0] read end (we read CGI output here), [1] write end (child stdout)
    bool        _pipes_open;

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
