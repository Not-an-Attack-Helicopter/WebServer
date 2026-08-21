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

static const int CGI_TIMEOUT_MS = 10000;

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

CGIResult run_cgi(const std::string& path, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, const std::string& input, const std::string& working_dir)
{
    CGIResult result;

    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1)
        return result;

    pid_t pid = fork();
    if (pid == -1) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process
        // Redirect stdin/stdout
        close(in_pipe[1]);
        close(out_pipe[0]);
        if (dup2(in_pipe[0], STDIN_FILENO) == -1) _exit(127);
        if (dup2(out_pipe[1], STDOUT_FILENO) == -1) _exit(127);
        close(in_pipe[0]); close(out_pipe[1]);
        // Change working directory if requested
        if (!working_dir.empty()) {
            if (chdir(working_dir.c_str()) != 0) _exit(126);
        }

        // Build argv array in child
        std::vector<char*> argv;
        if (args.empty()) {
            argv.push_back(const_cast<char*>(path.c_str()));
        } else {
            for (size_t i = 0; i < args.size(); ++i)
                argv.push_back(const_cast<char*>(args[i].c_str()));
        }
        argv.push_back(NULL);

        // Build envp array in child
        std::vector<std::string> env_strings;
        env_strings.reserve(env.size());
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
            env_strings.push_back(it->first + "=" + it->second);
        std::vector<char*> envp;
        envp.reserve(env_strings.size() + 1);
        for (size_t i = 0; i < env_strings.size(); ++i)
            envp.push_back(const_cast<char*>(env_strings[i].c_str()));
        envp.push_back(NULL);

        // Execve
        execve(path.c_str(), argv.empty() ? NULL : &argv[0], envp.empty() ? NULL : &envp[0]);
        // If execve fails
        _exit(127);
    }

    // Parent: close unused ends and make the pipe endpoints non-blocking.
    close(in_pipe[0]);
    close(out_pipe[1]);

    if (fcntl(in_pipe[1], F_SETFL, O_NONBLOCK) == -1 ||
        fcntl(out_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        close(in_pipe[1]);
        close(out_pipe[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return result;
    }

    // Write stdin and read stdout concurrently. Doing either operation to
    // completion before the other can deadlock on a full pipe.
    std::string out;
    const size_t BUF_SZ = 4096;
    char buf[BUF_SZ];
    size_t input_offset = 0;
    bool input_open = true;
    bool output_open = true;
    bool timed_out = false;
    int elapsed = 0;
    if (input.empty()) {
        close(in_pipe[1]);
        input_open = false;
    }
    while (input_open || output_open) {
        struct pollfd fds[2];
        nfds_t count = 0;
        int input_index = -1;
        int output_index = -1;

        if (input_open) {
            input_index = (int)count;
            fds[count].fd = in_pipe[1];
            fds[count].events = (input_offset < input.size()) ? POLLOUT : 0;
            fds[count].revents = 0;
            ++count;
        }
        if (output_open) {
            output_index = (int)count;
            fds[count].fd = out_pipe[0];
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
            elapsed += 100;
            if (elapsed >= CGI_TIMEOUT_MS) {
                timed_out = true;
                break;
            }
            continue;
        }
        elapsed = 0;

        if (input_index != -1 && (fds[input_index].revents & (POLLOUT | POLLERR | POLLHUP))) {
            if (input_offset < input.size()) {
                ssize_t written = write(in_pipe[1], input.data() + input_offset,
                                        input.size() - input_offset);
                if (written > 0)
                    input_offset += (size_t)written;
                else if (written == -1 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                    input_open = false;
            }
            if (input_offset == input.size()) {
                close(in_pipe[1]);
                input_open = false;
            }
        }

        if (output_index != -1 && (fds[output_index].revents & (POLLIN | POLLERR | POLLHUP))) {
            while (true) {
                ssize_t r = read(out_pipe[0], buf, BUF_SZ);
                if (r > 0) out.append(buf, buf + r);
                else if (r == -1 && errno == EINTR) continue;
                else if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                else {
                    output_open = false;
                    close(out_pipe[0]);
                    break;
                }
            }
        }
    }

    if (input_open) close(in_pipe[1]);
    if (output_open) close(out_pipe[0]);

    // Wait for child
    int status = 0;
    if (timed_out) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        result.exit_code = -SIGKILL;
    }
    pid_t w = timed_out ? pid : waitpid(pid, &status, 0);
    if (w == pid) {
        if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) result.exit_code = -WTERMSIG(status);
        else result.exit_code = -1;
    }

    result.raw_output = out;

    // Parse headers and body from CGI stdout
    size_t hdr_end = out.find("\r\n\r\n");
    size_t sep_len = 4;
    if (hdr_end == std::string::npos) {
        hdr_end = out.find("\n\n");
        sep_len = 2;
    }
    std::string hdrs;
    if (hdr_end != std::string::npos) {
        hdrs = out.substr(0, hdr_end);
        result.body = out.substr(hdr_end + sep_len);
    } else {
        // No headers found: entire output is body
        result.body = out;
        return result;
    }

    // Split header lines and parse
    std::istringstream ss(hdrs);
    std::string line;
    while (std::getline(ss, line)) {
        // Remove trailing CR
        if (!line.empty() && line[line.size()-1] == '\r') line.resize(line.size()-1);
        if (line.empty()) continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon+1));
        std::string key_l = to_lower(key);
        result.headers[key_l] = val;
        if (key_l == "status") {
            // Status: 201 Created OR Status: 200
            std::istringstream s2(val);
            int st; s2 >> st;
            if (s2) result.status = st;
        }
    }

    return result;
}
