#pragma once
#include "CgiProcess.hpp"
#include "Buffer.hpp"
#include "HTTPRequest.hpp"

// true if the request's path matches a configured CGI extension; also sets
// request.cgi.binary_path as a side effect
bool hasCGIExtension(HTTPRequest& request);

// body's fully in by now, build the process and kick it off. Not spawned
// yet, that + epoll registration is still ahead.
StatusCode handleCGI(HTTPRequest& request, HTTPResponse& response,
					 const Config::Socket& socket);

class CgiHandler {
public:

	enum ScriptState {
		WRITING_PIPES, // feeding the CGI's stdin (request body)
		PROCESSING,    // stdin closed, waiting for output to start
		READING_PIPES, // draining the CGI's stdout
		COMPLETE,
		ERROR
	};

	CgiHandler(CgiProcess* process, const std::string& body); // takes ownership of process
	~CgiHandler();

	ScriptState state(void) const;

	void readStdout(void); // one non-blocking read attempt, moves PROCESSING -> READING_PIPES

	// only does anything once state() == COMPLETE
	void buildResponse(HTTPResponse& response, bool headers_only) const;

private:
	CgiHandler(const CgiHandler&);
	CgiHandler& operator=(const CgiHandler&);

	CgiProcess* _process;
	Buffer      _instream;  // -> process stdin
	Buffer      _outstream; // <- process stdout
	ScriptState _state;
};
