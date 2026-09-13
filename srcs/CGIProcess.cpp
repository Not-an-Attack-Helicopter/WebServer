/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gpochon, bstorck <marvin@42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 08:42:40 by gpochon           #+#    #+#             */
/*   Updated: 2026/09/13 08:42:41 by gpochon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/CGIProcess.hpp"
#include "../incs/templates.hpp"
#include "../incs/constexpr.hpp"
// #include "../incs/Logger.hpp"
#include "../incs/utils.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

static const int CGI_TIMEOUT_S = 10;

CGIProcess::CGIProcess(const std::string& path, const std::vector<std::string>& args,
                       const std::map<std::string, std::string>& env,
                       const std::string& working_dir)
	: _path(path), _args(args), _env(env), _working_dir(working_dir),
	_pipes_open(false),
	_pid(-1), _stdin_fd(-1), _stdout_fd(-1),
	_reaped(false), _exit_code(-1), _deadline(0),
	// _state(WRITING_PIPES),
	_status(OK), _content_type("text/html"),
	_has_status(false), _has_location(false), _headers_done(false),
	// _line_ending(NONE), _line_end_size(0) {
	_line_end_size(0), _line_ending("") {

	_in_pipe[0] = -1; _in_pipe[1] = -1;
	_out_pipe[0] = -1; _out_pipe[1] = -1;

	if (pipe(_in_pipe) == -1)
		return;
	if (pipe(_out_pipe) == -1) {
		close(_in_pipe[0]); close(_in_pipe[1]);
		_in_pipe[0] = -1; _in_pipe[1] = -1;
		return;
	}

	_pipes_open = true;
}

bool CGIProcess::spawn() {

	if (!_pipes_open || _pid != -1)
		return false;

	pid_t pid = fork();
	if (pid == -1) {
		close(_in_pipe[0]); close(_in_pipe[1]);
		close(_out_pipe[0]); close(_out_pipe[1]);
		_in_pipe[0] = -1; _in_pipe[1] = -1;
		_out_pipe[0] = -1; _out_pipe[1] = -1;
		_pipes_open = false;
		return false;
	}

	if (pid == 0) {
		close(_in_pipe[1]);
		close(_out_pipe[0]);
		if (dup2(_in_pipe[0], STDIN_FILENO) == -1) _exit(127);
		if (dup2(_out_pipe[1], STDOUT_FILENO) == -1) _exit(127);
		close(_in_pipe[0]); close(_out_pipe[1]);
		if (!_working_dir.empty()) {
			if (chdir(_working_dir.c_str()) != 0) _exit(126);
		}

		std::vector<char*> argv;
		if (_args.empty()) {
			argv.push_back(const_cast<char*>(_path.c_str()));
		} else {
			for (size_t i = 0; i < _args.size(); ++i)
				argv.push_back(const_cast<char*>(_args[i].c_str()));
		}
		argv.push_back(NULL);

		std::vector<std::string> env_strings;
		env_strings.reserve(_env.size());
		for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it)
			env_strings.push_back(it->first + "=" + it->second);
		std::vector<char*> envp;
		envp.reserve(env_strings.size() + 1);
		for (size_t i = 0; i < env_strings.size(); ++i)
			envp.push_back(const_cast<char*>(env_strings[i].c_str()));
		envp.push_back(NULL);

		execve(_path.c_str(), &argv[0], &envp[0]);
		_exit(127);
	}

	close(_in_pipe[0]);
	close(_out_pipe[1]);
	_in_pipe[0] = -1;
	_out_pipe[1] = -1;

	// Let's try having to server set the pipe ends to non-blocking.
	// If it that's too late, set them to non-blocking here.
	// TEST
	// if (fcntl(_in_pipe[1], F_SETFL, O_NONBLOCK) == -1 ||
	//     fcntl(_out_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
	//     close(_in_pipe[1]);
	//     close(_out_pipe[0]);
	//     _in_pipe[1] = -1;
	//     _out_pipe[0] = -1;
	//     kill(pid, SIGKILL);
	//     waitpid(pid, NULL, 0);
	//     return false;
	// }

	_pid = pid;
	_stdin_fd = _in_pipe[1];
	_stdout_fd = _out_pipe[0];
	_in_pipe[1] = -1;  // ownership to _stdin_fd
	_out_pipe[0] = -1; // ownership to _stdout_fd
	_deadline = std::time(NULL) + CGI_TIMEOUT_S;

    return true;
}

CGIProcess::~CGIProcess() {
	if (_stdin_fd != -1) close(_stdin_fd);
	if (_stdout_fd != -1) close(_stdout_fd);
	if (_in_pipe[0] != -1) close(_in_pipe[0]);
	if (_in_pipe[1] != -1) close(_in_pipe[1]);
	if (_out_pipe[0] != -1) close(_out_pipe[0]);
	if (_out_pipe[1] != -1) close(_out_pipe[1]);
	if (_pid != -1 && !_reaped) {
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
	}
}

bool  CGIProcess::valid()     const { return _pipes_open; }
pid_t CGIProcess::pid()       const { return _pid; }
int   CGIProcess::stdinFd()         { return _stdin_fd; }
int   CGIProcess::stdoutFd()  const { return _stdout_fd; }

void CGIProcess::closeStdin() {
	if (_stdin_fd != -1) {
		close(_stdin_fd);
		_stdin_fd = -1;
	}
}

void CGIProcess::closeStdout() {
	if (_stdout_fd != -1) {
		close(_stdout_fd);
		_stdout_fd = -1;
	}
}


// fd always equals stdoutFd(); readStdout() does the actual fetch, parses
// whatever's complete, and detects COMPLETE/ERROR
// ssize_t CGIProcess::queueIncomingData(int fd) {
// 	(void)fd;
// 	readStdout();
// 	return 0;
// }


// same shape as handleWritable(), just using _instream
// of the raw _input string
// void CGIProcess::writeStdin() {
//
// 	if (_state != WRITING_PIPES)
// 		return;
//
// 	if (_instream.begin == _instream.end) {
// 		closeStdin();
// 		_state = PROCESSING;
// 		return;
// 	}
//
// 	ssize_t written = _instream.flushData(stdinFd(), true);
//
// 	// poll() already told us this fd is ready; treat any -1 as fatal,
// 	// same as Server::_handleSocketReadEvent() does for sockets. Not
// 	// allowed to branch on errno's value to decide what to do next.
// 	if (written == -1) {
// 		_state = ERROR;
// 		return;
// 	}
//
// 	if (_instream.begin == _instream.end) {
// 		closeStdin();
// 		_state = PROCESSING;
// 	}
//
// }

// same shape as writeStdin(), just reading into _outstream instead
ssize_t CGIProcess::queueIncomingData(int fd) {

	// nothing else moves us from PROCESSING to READING_PIPES, do it here
	// if (_state == PROCESSING)
	// 	_state = READING_PIPES;

	// if (_state != READING_PIPES)
	// 	return;

	// Problematic, because caller can not differentiate this from got == 0
	// will think about this during the day...
	// buffer full, not necessarily eof, wait for something to drain it
	// if (_outstream.end == _outstream.data.size())
	// 	return 0;

	// I know it is always stdoutFd()...
	// but when we get passed the int why fetch it again via stdoutFd()
	// just using the int directly is faster
	// ssize_t got = _outstream.fetchData(fd, true);
 //
	// if (got == -1) {
 //        // server closes fd
	// 	// closeStdout();
	// 	_state = ERROR;
	// }
 //
	// // if (got > 0)
	// // 	_consumeAvailableOutput();
 //
	// if (got == 0) {
	// 	// _consumeAvailableOutput(); // fold in whatever's left before COMPLETE
	// 	// closeStdout();
	// 	_state = COMPLETE;
	// }
 //
 //    return got;
    return _outstream.fetchData(fd, true);

}

static inline bool isLineWS(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool isIgnored(const std::string& name) {
	/*
	 * The HTTP server controls message framing and connection
	 * management. These are not copied from CGI.
	 */
	return equalCI(name, "Content-Length") ||
		   equalCI(name, "Transfer-Encoding") ||
		   equalCI(name, "Connection") ||
		   equalCI(name, "Keep-Alive") ||
		   equalCI(name, "Upgrade") ||
		   equalCI(name, "TE") ||
		   equalCI(name, "Trailer");
}

std::size_t CGIProcess::_findHeaderLineEnd() {

	ssize_t LF_pos = _outstream.find(HTTP::LF);
	if (LF_pos == -1) return std::string::npos;
	if (LF_pos != 0 && _outstream.data[LF_pos - 1] == HTTP::CR) {
		// _line_ending = CRLF;
		_line_ending = HTTP::CRLF;
		_line_end_size = CRLF_SIZE;
		return static_cast<std::size_t>(LF_pos) - 1;
   }

	// _line_ending = LF;
	_line_ending = HTTP::LF;
	_line_end_size = LF_SIZE;
	return static_cast<std::size_t>(LF_pos);
}

// trims directly against the buffer first, so there's only one extraction
// (already trimmed) instead of one to pull the raw line out and another
// inside trim()
bool CGIProcess::_consumeHeaderLine() {

	size_t line_end_pos = 0;
	// if (_line_ending == NONE) {
	// 	line_end_pos = _findHeaderLineEnd();
	// } else if (_line_ending == CRLF) {
	// 	line_end_pos = _outstream.find(http::CRLF);
	// } else {
	// 	line_end_pos = _outstream.find(http::LF);
	// }
	if (_line_ending.empty()) {
		line_end_pos = _findHeaderLineEnd();
	} else {
		line_end_pos = _outstream.find(_line_ending);
	}

	if (line_end_pos == 0) {

		// blank line, headers are done
		_outstream.mark = _outstream.begin + _line_end_size;
		_headers_done = true;
		return true;

	} else if (line_end_pos == std::string::npos) {

		_outstream.mark = _outstream.begin;
		return false;

	} else {

		// RFC 3875: No whitespace before the field-name and before the colon.
		if (isLineWS(_outstream.data[_outstream.begin])) {
			throw std::runtime_error("CGI response: no whitespaces in field-name allowed");
		}

		// Find separator ':'
		std::size_t colon_pos = static_cast<std::size_t>(_outstream.find(':'));
		if (colon_pos == 0) {
			throw std::runtime_error("CGI response: header field has no field-name");
		}
		if (colon_pos == line_end_pos || colon_pos == std::string::npos) {
			throw std::runtime_error("CGI response: header field has no colon");
		}

		// field-name is a token: no whitespace or separators allowed, so this
		// also catches "Content-Type : text/plain" (space before the colon
		// ends up inside key, and ' ' isn't a tchar)
		for (std::size_t i = 0; i < colon_pos; ++i) {
			if (!isTChar(_outstream.data[_outstream.begin + i])) {
				std::ostringstream oss;
				oss << "CGI response: invalid character or whitespace in header field-name\n";
				oss << "culprit: {" + i2a((int)(unsigned char)_outstream.data[_outstream.begin + i]) + "}";
				throw std::runtime_error(oss.str());
			}
		}

		// trim header field-value from whitespaces
		std::size_t value_first = colon_pos + 1;
		while (value_first < line_end_pos && isLineWS(_outstream.data[_outstream.begin + value_first]))
			++value_first;
		std::size_t value_last = line_end_pos;
		while (value_last > value_first && isLineWS(_outstream.data[_outstream.begin + value_last - 1]))
			--value_last;

		std::string key = _outstream.substr(0, colon_pos);
		std::string value = _outstream.substr(value_first, value_last);

		if (equalCI(key, "Status")) {
			int code = std::atoi(value.c_str());
			if (code >= 100 && code <= 599) {
				_status = static_cast<StatusCode>(code);
				_has_status = true;
			}
		}
		if (equalCI(key, "Content-Type")) _content_type = value;
		if (equalCI(key, "Location")) _has_location = true;
		if (!isIgnored(key)) _headers[key] = value;

		_outstream.mark = _outstream.begin + line_end_pos + _line_end_size;
		return true;
	}
}

// consumes whatever complete lines are in _outstream, same technique as
// parseRequestLine()/parseHeaders(), just for cgi output
void CGIProcess::consumeAvailableOutput() {

	while (!_headers_done && _outstream.mark < _outstream.end) {

		bool has_consumed_line;
		try {
			has_consumed_line = _consumeHeaderLine();
		} catch (std::exception& e) {
			throw std::runtime_error(e.what());
		}
		if (has_consumed_line == true) {
			_outstream.begin = _outstream.mark;
		}
	}

	if (_headers_done) {
		_body += _outstream.substr(0);
		_outstream.reset();
		return;
	}

	if (_outstream.begin == _outstream.end) {
		_outstream.reset();
	} else if (_instream.end == _instream.data.size()) {
		if (_outstream.begin > 0) {
			// free up what we already committed past
			_outstream.compact();
		} else {
			throw std::runtime_error("read stdout: buffer overflow");
		}
	}

    return;
}

// headers/body/status were already parsed incrementally as bytes arrived
// (see _consumeAvailableOutput()), this just transfers them onto response
void CGIProcess::buildResponse(HTTPResponse& response, bool headers_only) const {

	// a broken pipe read/write, or a malformed header line (bad field-name
	// char), lands here instead of COMPLETE
	// if (_state == ERROR) {
	//     response.setStatus(INTERNAL_SERVER_ERROR);
	//     response.setBody("", HEAP, "", headers_only);
	//     return;
	// }

	// if (_state != COMPLETE)
	//     return;

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it) {
		response.setHeader(it->first, it->second);
	}

	StatusCode status = _status;
	// CGI/1.1: Location with no Status means client redirect
	if (_has_location && !_has_status)
		status = FOUND;

	response.setStatus(status);
	response.setBody(_body, HEAP, _content_type, headers_only);

}

bool  CGIProcess::wantsWrite() const { return _stdin_fd != -1; }
bool  CGIProcess::wantsRead()  const { return _stdout_fd != -1; }
bool  CGIProcess::isDone()     const { return _stdin_fd == -1 && _stdout_fd == -1 && _reaped; }
bool  CGIProcess::isExpired(const std::time_t now) const { return _pid != -1 && now >= _deadline; }

// deprecated
// void CGIProcess::handleWritable() {
//     if (_stdin_fd == -1)
//         return;
//     if (_input_offset >= _input.size()) {
//         close(_stdin_fd);
//         _stdin_fd = -1;
//         return;
//     }
//     ssize_t written = write(_stdin_fd, _input.data() + _input_offset, _input.size() - _input_offset);
//     if (written > 0)
//         _input_offset += (size_t)written;
//     else if (written == -1 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
//         close(_stdin_fd);
//         _stdin_fd = -1;
//         return;
//     }
//     if (_input_offset == _input.size()) {
//         close(_stdin_fd);
//         _stdin_fd = -1;
//     }
// }

// deprecated
// void CGIProcess::handleReadable() {
//     if (_stdout_fd == -1)
//         return;
//     const size_t BUF_SZ = 4096;
//     char buf[BUF_SZ];
//     while (true) {
//         ssize_t r = read(_stdout_fd, buf, BUF_SZ);
//         if (r > 0) {
//             _output.append(buf, buf + r);
//             continue;
//         }
//         if (r == -1 && errno == EINTR)
//             continue;
//         if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
//             return;
//         close(_stdout_fd);
//         _stdout_fd = -1;
//         return;
//     }
// }

bool CGIProcess::tryReap(bool block) {

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

void CGIProcess::forceKill() {
	if (_pid != -1)
		kill(_pid, SIGKILL);
}

// deprecated??
// CGIResult CGIProcess::result() const {
//
//     CGIResult res;
//     res.exit_code = _exit_code;
//     res.raw_output = _output;
//
//     size_t hdr_end = _output.find("\r\n\r\n");
//     size_t sep_len = 4;
//     if (hdr_end == std::string::npos) {
//         hdr_end = _output.find("\n\n");
//         sep_len = 2;
//     }
//     if (hdr_end == std::string::npos) {
//         res.body = _output;
//         return res;
//     }
//     res.body = _output.substr(hdr_end + sep_len);
//
//     std::istringstream ss(_output.substr(0, hdr_end));
//     std::string line;
//     while (std::getline(ss, line)) {
//         if (!line.empty() && line[line.size()-1] == '\r') line.resize(line.size()-1);
//         if (line.empty()) continue;
//         size_t colon = line.find(':');
//         if (colon == std::string::npos) continue;
//         std::string key = trim(line.substr(0, colon));
//         std::string val = trim(line.substr(colon+1));
//         std::string key_l = tolowerASCII(key);
//         res.headers[key_l] = val;
//         if (key_l == "status") {
//             std::istringstream s2(val);
//             int st; s2 >> st;
//             if (s2) res.status = st;
//         }
//     }
//     return res;
// }

// CGIResult run_cgi(const std::string& path, const std::vector<std::string>& args,
//                    const std::map<std::string, std::string>& env,
//                    const std::string& working_dir) {
//
//     CGIProcess proc(path, args, env, working_dir);
//     if (!proc.valid())
//         return CGIResult();
//
//     while (!proc.isDone()) {
//         if (!proc.wantsWrite() && !proc.wantsRead()) {
//             proc.tryReap(true);
//             break;
//         }
//
//         struct pollfd fds[2];
//         nfds_t count = 0;
//         int write_idx = -1;
//         int read_idx = -1;
//
//         if (proc.wantsWrite()) {
//             write_idx = (int)count;
//             fds[count].fd = proc.stdinFd();
//             fds[count].events = POLLOUT;
//             fds[count].revents = 0;
//             ++count;
//         }
//         if (proc.wantsRead()) {
//             read_idx = (int)count;
//             fds[count].fd = proc.stdoutFd();
//             fds[count].events = POLLIN;
//             fds[count].revents = 0;
//             ++count;
//         }
//
//         int ready = poll(fds, count, 100);
//         if (ready == -1) {
//             if (errno == EINTR) continue;
//             break;
//         }
//         if (ready == 0) {
//             if (proc.isExpired(std::time(NULL)))
//                 proc.forceKill();
//             continue;
//         }
//
//         if (write_idx != -1 && (fds[write_idx].revents & (POLLOUT | POLLERR | POLLHUP)))
//             proc.handleWritable();
//         if (read_idx != -1 && (fds[read_idx].revents & (POLLIN | POLLERR | POLLHUP)))
//             proc.handleReadable();
//     }
//
//     proc.tryReap(true);
//     return proc.result();
// }
