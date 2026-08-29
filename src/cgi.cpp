#include "cgi.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <sstream>

static const int CGI_TIMEOUT_S = 10;

static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static inline std::string trim(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    size_t e = s.size();
    while (e > b && (s[e-1] == ' ' || s[e-1] == '\t' || s[e-1] == '\r' || s[e-1] == '\n')) --e;
    return s.substr(b, e-b);
}

CgiProcess::CgiProcess(const std::string& path, const std::vector<std::string>& args,
                        const std::map<std::string, std::string>& env,
                        const std::string& input, const std::string& working_dir)
    : _pid(-1), _stdin_fd(-1), _stdout_fd(-1), _input(input), _input_offset(0),
      _reaped(false), _exit_code(-1), _deadline(0)
{
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1)
        return;
    if (pipe(out_pipe) == -1) {
        close(in_pipe[0]); close(in_pipe[1]);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return;
    }

    if (pid == 0) {
        close(in_pipe[1]);
        close(out_pipe[0]);
        if (dup2(in_pipe[0], STDIN_FILENO) == -1) _exit(127);
        if (dup2(out_pipe[1], STDOUT_FILENO) == -1) _exit(127);
        close(in_pipe[0]); close(out_pipe[1]);
        if (!working_dir.empty()) {
            if (chdir(working_dir.c_str()) != 0) _exit(126);
        }

        std::vector<char*> argv;
        if (args.empty()) {
            argv.push_back(const_cast<char*>(path.c_str()));
        } else {
            for (size_t i = 0; i < args.size(); ++i)
                argv.push_back(const_cast<char*>(args[i].c_str()));
        }
        argv.push_back(NULL);

        std::vector<std::string> env_strings;
        env_strings.reserve(env.size());
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
            env_strings.push_back(it->first + "=" + it->second);
        std::vector<char*> envp;
        envp.reserve(env_strings.size() + 1);
        for (size_t i = 0; i < env_strings.size(); ++i)
            envp.push_back(const_cast<char*>(env_strings[i].c_str()));
        envp.push_back(NULL);

        execve(path.c_str(), &argv[0], &envp[0]);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    if (fcntl(in_pipe[1], F_SETFL, O_NONBLOCK) == -1 ||
        fcntl(out_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        close(in_pipe[1]);
        close(out_pipe[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return;
    }

    _pid = pid;
    _stdin_fd = in_pipe[1];
    _stdout_fd = out_pipe[0];
    _deadline = time(NULL) + CGI_TIMEOUT_S;

    if (_input.empty()) {
        close(_stdin_fd);
        _stdin_fd = -1;
    }
}

CgiProcess::~CgiProcess() {
    if (_stdin_fd != -1) close(_stdin_fd);
    if (_stdout_fd != -1) close(_stdout_fd);
    if (_pid != -1 && !_reaped) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
    }
}

bool  CgiProcess::valid()     const { return _pid != -1; }
pid_t CgiProcess::pid()       const { return _pid; }
int   CgiProcess::stdinFd()   const { return _stdin_fd; }
int   CgiProcess::stdoutFd()  const { return _stdout_fd; }
bool  CgiProcess::wantsWrite() const { return _stdin_fd != -1; }
bool  CgiProcess::wantsRead()  const { return _stdout_fd != -1; }
bool  CgiProcess::isDone()     const { return _stdin_fd == -1 && _stdout_fd == -1 && _reaped; }
bool  CgiProcess::isExpired(time_t now) const { return _pid != -1 && now >= _deadline; }

void CgiProcess::handleWritable() {
    if (_stdin_fd == -1)
        return;
    if (_input_offset >= _input.size()) {
        close(_stdin_fd);
        _stdin_fd = -1;
        return;
    }
    ssize_t written = write(_stdin_fd, _input.data() + _input_offset, _input.size() - _input_offset);
    if (written > 0)
        _input_offset += (size_t)written;
    else if (written == -1 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        close(_stdin_fd);
        _stdin_fd = -1;
        return;
    }
    if (_input_offset == _input.size()) {
        close(_stdin_fd);
        _stdin_fd = -1;
    }
}

void CgiProcess::handleReadable() {
    if (_stdout_fd == -1)
        return;
    const size_t BUF_SZ = 4096;
    char buf[BUF_SZ];
    while (true) {
        ssize_t r = read(_stdout_fd, buf, BUF_SZ);
        if (r > 0) {
            _output.append(buf, buf + r);
            continue;
        }
        if (r == -1 && errno == EINTR)
            continue;
        if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;
        close(_stdout_fd);
        _stdout_fd = -1;
        return;
    }
}

bool CgiProcess::tryReap(bool block) {
    if (_reaped)
        return true;
    if (_pid == -1) {
        _reaped = true;
        return true;
    }
    int status = 0;
    pid_t w = waitpid(_pid, &status, block ? 0 : WNOHANG);
    if (w == 0)
        return false;
    _reaped = true;
    if (w == _pid) {
        if (WIFEXITED(status)) _exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) _exit_code = -WTERMSIG(status);
        else _exit_code = -1;
    }
    return true;
}

void CgiProcess::forceKill() {
    if (_pid != -1)
        kill(_pid, SIGKILL);
}

CGIResult CgiProcess::result() const {
    CGIResult res;
    res.exit_code = _exit_code;
    res.raw_output = _output;

    size_t hdr_end = _output.find("\r\n\r\n");
    size_t sep_len = 4;
    if (hdr_end == std::string::npos) {
        hdr_end = _output.find("\n\n");
        sep_len = 2;
    }
    if (hdr_end == std::string::npos) {
        res.body = _output;
        return res;
    }
    res.body = _output.substr(hdr_end + sep_len);

    std::istringstream ss(_output.substr(0, hdr_end));
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size()-1] == '\r') line.resize(line.size()-1);
        if (line.empty()) continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon+1));
        std::string key_l = to_lower(key);
        res.headers[key_l] = val;
        if (key_l == "status") {
            std::istringstream s2(val);
            int st; s2 >> st;
            if (s2) res.status = st;
        }
    }
    return res;
}

// Blocking convenience wrapper (standalone/offline use only, e.g. tests).
// The live server should drive a CgiProcess from its own epoll loop instead.
CGIResult run_cgi(const std::string& path, const std::vector<std::string>& args,
                   const std::map<std::string, std::string>& env,
                   const std::string& input, const std::string& working_dir)
{
    CgiProcess proc(path, args, env, input, working_dir);
    if (!proc.valid())
        return CGIResult();

    while (!proc.isDone()) {
        if (!proc.wantsWrite() && !proc.wantsRead()) {
            proc.tryReap(true);
            break;
        }

        struct pollfd fds[2];
        nfds_t count = 0;
        int write_idx = -1;
        int read_idx = -1;

        if (proc.wantsWrite()) {
            write_idx = (int)count;
            fds[count].fd = proc.stdinFd();
            fds[count].events = POLLOUT;
            fds[count].revents = 0;
            ++count;
        }
        if (proc.wantsRead()) {
            read_idx = (int)count;
            fds[count].fd = proc.stdoutFd();
            fds[count].events = POLLIN;
            fds[count].revents = 0;
            ++count;
        }

        int ready = poll(fds, count, 100);
        if (ready == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (ready == 0) {
            if (proc.isExpired(time(NULL)))
                proc.forceKill();
            continue;
        }

        if (write_idx != -1 && (fds[write_idx].revents & (POLLOUT | POLLERR | POLLHUP)))
            proc.handleWritable();
        if (read_idx != -1 && (fds[read_idx].revents & (POLLIN | POLLERR | POLLHUP)))
            proc.handleReadable();
    }

    proc.tryReap(true);
    return proc.result();
}
