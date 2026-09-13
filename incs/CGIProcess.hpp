/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIProcess.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gpochon <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 08:48:18 by gpochon           #+#    #+#             */
/*   Updated: 2026/09/13 08:48:19 by gpochon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

# include "HTTPResponse.hpp"
#include "Buffer.hpp"
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sys/types.h>

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
class CGIProcess {
public:

	// enum LineEnding {
	// 	NONE,
	// 	LF,
	// 	CRLF
	// };

	CGIProcess(const std::string& path,
			const std::vector<std::string>& args,
			const std::map<std::string, std::string>& env,
			const std::string& working_dir = "");
	~CGIProcess();

	bool  valid() const; // false if pipe() failed
	bool  spawn();        // fork()+execve()'s using the already-open pipes; false on failure
	pid_t pid()   const;
	int   stdinFd(); // -1 once the write end is closed
	int   stdoutFd() const; // -1 once the read end is closed

	// closes the fd and sets it to -1, same as handleWritable()/
	// handleReadable() do once done, so wantsWrite()/wantsRead()/isDone()
	// and the destructor stay correct either way
	void closeStdin();
	void closeStdout();

	ssize_t queueIncomingData(int fd);

	void consumeAvailableOutput();

	// void writeStdin(); // one non-blocking write attempt, WRITING_PIPES only
	// void readStdout();  // one non-blocking read attempt, moves PROCESSING -> READING_PIPES

	bool wantsWrite() const;
	bool wantsRead()  const;
	bool isDone()     const; // both pipe ends closed and child reaped

	void handleWritable(); // one non-blocking write attempt
	void handleReadable(); // drain readable bytes until EAGAIN/EOF/error
	bool tryReap(bool block = false); // waitpid; sets exit code once reaped

	bool isExpired(const std::time_t now) const;
	void forceKill(); // SIGKILL; caller still needs to tryReap()

	CGIResult result() const;

	// only does anything once _state == COMPLETE
	void buildResponse(HTTPResponse& response, bool headers_only) const;

private:
	CGIProcess(const CGIProcess&);
	CGIProcess& operator=(const CGIProcess&);

	std::size_t _findHeaderLineEnd();

	// parses the line sitting at _outstream.begin, line_len chars long
	// (not counting the '\n'), straight off the buffer -> _headers/etc.
	// returns true if it was the blank line ending the headers
	bool _consumeHeaderLine();
	// finds complete lines in _outstream, commits begin past each one;
	// once the blank line is hit, the rest becomes _body
	// void _consumeAvailableOutput();

	static const std::size_t           LF_SIZE = 1;
	static const std::size_t           CRLF_SIZE = 2;

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
	// std::string _input;
	// size_t      _input_offset;
	// std::string _output;
	bool        _reaped;
	int         _exit_code;
	std::time_t _deadline;

	Buffer      _instream;  // -> our stdin
	Buffer      _outstream; // <- our stdout
	// ScriptState _state;

	// response, parsed incrementally as bytes arrive
	std::map<std::string, std::string> _headers;
	StatusCode  _status;
	std::string _content_type;
	bool        _has_status;
	bool        _has_location;
	bool        _headers_done;
	// LineEnding  _line_ending;
	std::size_t _line_end_size;
	std::string _line_ending;
	std::string _body;
};

#endif

// deprecated: called handleWritable()/handleReadable()/result(), which are
// now commented out in CGIProcess.cpp (used the removed _input/_output)
// CGIResult run_cgi(const std::string& path,
//                   const std::vector<std::string>& args,
//                   const std::map<std::string, std::string>& env,
//                   const std::string& working_dir = "");
