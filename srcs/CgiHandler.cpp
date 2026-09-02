#include "../incs/CgiHandler.hpp"
#include "../incs/CgiEnv.hpp"
#include "../incs/Logger.hpp"
#include <cerrno>
#include <fstream>
#include <sstream>

bool hasCGIExtension(HTTPRequest& request) {

	const Config::Location& location = *request.resolved.location;
	const std::string& path = request.resolved.path;

	std::size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1) {
		return false;
	}

	const std::string ext = path.substr(dot);
	std::map<std::string, std::string>::const_iterator it = location.interpreters.find(ext);
	if (it != location.interpreters.end()) {
		request.cgi.binary_path = it->second;
		return true;
	}

	return false;

}

// argv for execve
static std::vector<std::string> buildCgiArgs(const HTTPRequest& request) {

	std::vector<std::string> args;
	args.push_back(request.cgi.binary_path);
	args.push_back(request.resolved.path);
	return args;

}

// read the body back off disk, that's our cgi stdin
static std::string readCgiInput(const HTTPRequest& request) {

	// small bodies live in memory now (Sink::HEAP), only large ones spool
	// to disk -- see the switch on request.body.sink in _parseBody()
	if (request.body.sink == HEAP)
		return request.body.temp;

	if (request.body.path.empty())
		return "";

	std::ifstream file(request.body.path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";

	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();

}

StatusCode handleCGI(HTTPRequest& request, HTTPResponse& response,
					 const Config::Socket& socket) {

	(void)response; // filled in once the CGI actually runs

	std::string cgi_input = readCgiInput(request);
	std::vector<std::string> cgi_args = buildCgiArgs(request);
	std::string working_dir = request.resolved.path.substr(0, request.resolved.path.find_last_of('/'));

	std::map<std::string, std::string> env = build_cgi_env(request, socket,
															*request.resolved.domain,
															*request.resolved.location,
															request.resolved.path);

	request.cgi_process = new CgiProcess(request.cgi.binary_path, cgi_args, env, cgi_input, working_dir);

	if (!request.cgi_process->valid()) {
		log.error("cgi: failed to open pipes for " + request.cgi.binary_path);
		return INTERNAL_SERVER_ERROR;
	}

	return NO_STATUS;

}

CgiHandler::CgiHandler(CgiProcess* process, const std::string& body)
	: _process(process), _state(WRITING_PIPES)
{
	if (!body.empty()) {
		_instream.data.assign(body.begin(), body.end());
		_instream.end = body.size();
	}
}

CgiHandler::~CgiHandler() {
	delete _process;
}

CgiHandler::ScriptState CgiHandler::state(void) const {
	return _state;
}

// same shape as CgiProcess::handleWritable(), just writing from _instream
// (a Buffer) instead of the raw _input string
void CgiHandler::writeStdin(void) {

	if (_state != WRITING_PIPES)
		return;

	if (_instream.begin == _instream.end) {
		_process->closeStdin();
		_state = PROCESSING;
		return;
	}

	ssize_t written = _instream.flushData(_process->stdinFd(), true);

	if (written == -1 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
		_state = ERROR;
		return;
	}

	if (_instream.begin == _instream.end) {
		_process->closeStdin();
		_state = PROCESSING;
	}

}

// same shape as writeStdin(), just filling _outstream from the fd instead
// of draining _instream into it
void CgiHandler::readStdout(void) {

	// nothing else moves us from PROCESSING to READING_PIPES, do it here
	if (_state == PROCESSING)
		_state = READING_PIPES;

	if (_state != READING_PIPES)
		return;

	// buffer full, not necessarily eof, wait for something to drain it
	if (_outstream.end == _outstream.data.size())
		return;

	ssize_t got = _outstream.fetchData(_process->stdoutFd(), true);

	if (got == -1 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
		_state = ERROR;
		return;
	}

	if (got == 0) {
		_process->closeStdout();
		_state = COMPLETE;
	}

}
