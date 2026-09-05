#include "../incs/CgiHandler.hpp"
#include "../incs/CgiEnv.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>

bool hasCGIExtension(HTTPRequest& request) {

	const Config::Location& location = *request.resolved.location;
	const std::string& path = request.resolved.filepath;

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
	args.push_back(request.resolved.filepath);
	return args;

}

// read the body back off disk, that's our cgi stdin
static std::string readCgiInput(const HTTPRequest& request) {

	// small bodies live in memory, only big ones spool to disk
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

	(void)response; // filled in later, by CgiHandler::buildResponse()

	std::string cgi_input = readCgiInput(request);
	std::vector<std::string> cgi_args = buildCgiArgs(request);
	std::string working_dir = request.resolved.filepath.substr(0, request.resolved.filepath.find_last_of('/'));

	std::map<std::string, std::string> env = build_cgi_env(request, socket,
															*request.resolved.domain,
															*request.resolved.location,
															request.resolved.filepath);

	CgiProcess* process = new CgiProcess(request.cgi.binary_path, cgi_args, env, cgi_input, working_dir);

	if (!process->valid()) {
		log.error("cgi: failed to open pipes for " + request.cgi.binary_path);
		delete process;
		return INTERNAL_SERVER_ERROR;
	}

	if (!process->spawn()) {
		log.error("cgi: failed to spawn " + request.cgi.binary_path);
		delete process;
		return INTERNAL_SERVER_ERROR;
	}

	// epoll registration is still ahead, server side
	request.cgi_handler = new CgiHandler(process, cgi_input);

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

// cgi output is "headers, blank line, body" -- same split as before
void CgiHandler::buildResponse(HTTPResponse& response, bool headers_only) const {

	if (_state != COMPLETE)
		return;

	std::string raw = _outstream.str();

	std::size_t sep = raw.find("\r\n\r\n");
	std::size_t sep_len = 4;
	if (sep == std::string::npos) {
		sep = raw.find("\n\n");
		sep_len = 2;
	}

	std::string header_block = (sep == std::string::npos) ? "" : raw.substr(0, sep);
	std::string body = (sep == std::string::npos) ? raw : raw.substr(sep + sep_len);

	StatusCode status = OK;
	std::string content_type = "text/html";
	bool has_status = false;
	bool has_location = false;

	std::istringstream ss(header_block);
	std::string line;
	while (std::getline(ss, line)) {

		line = trim(line);
		if (line.empty())
			continue;

		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		std::string value = trim(line.substr(colon + 1));
		std::string key_lower = tolowerASCII(key);

		if (key_lower == "status") {
			int code = std::atoi(value.c_str());
			if (code >= 100 && code <= 599) {
				status = static_cast<StatusCode>(code);
				has_status = true;
			}
			continue;
		}
		if (key_lower == "content-type")
			content_type = value;
		if (key_lower == "location")
			has_location = true;

		response.setHeader(key, value);
	}

	// CGI/1.1: Location with no Status means client redirect
	if (has_location && !has_status)
		status = FOUND;

	response.setStatus(status);
	response.setBody(body, HEAP, content_type, headers_only);

}

// same shape as CgiProcess::handleWritable(), just using _instream
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

// same shape as writeStdin(), just reading into _outstream instead
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
