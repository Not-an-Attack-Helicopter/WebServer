#include "cgi.hpp"
#include <unistd.h>

CGIResult run_cgi(const std::string& path, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, const std::string& input)
{
    CGIResult result;
    result.exit_code = -1;
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1)
        return result;
    pid_t pid = fork();
    if (pid == -1) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return result;
    }
    if (pid == 0) {
        // child: become the CGI program
        close(in_pipe[1]);
        close(out_pipe[0]);
        if (dup2(in_pipe[0], STDIN_FILENO) == -1)
            _exit(1);
        if (dup2(out_pipe[1], STDOUT_FILENO) == -1)
            _exit(1);
        close(in_pipe[0]);
        close(out_pipe[1]);
        // todo: build argv/envp, then execve
        (void)path;
        (void)args;
        (void)env;
        _exit(1);
    }
    // parent: feed input in, read the output back, wait for the child
    (void)input;
    return result;
}
