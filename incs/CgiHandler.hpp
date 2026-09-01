#pragma once
#include "CgiProcess.hpp"
#include "Buffer.hpp"

class CgiHandler {
public:

	enum ScriptState {
		WRITING_PIPES, // feeding the CGI's stdin (request body)
		PROCESSING,    // stdin closed, waiting for output to start
		READING_PIPES, // draining the CGI's stdout
		COMPLETE,
		ERROR
	};

	explicit CgiHandler(CgiProcess* process); // takes ownership
	~CgiHandler();

	ScriptState state(void) const;

private:
	CgiHandler(const CgiHandler&);
	CgiHandler& operator=(const CgiHandler&);

	CgiProcess* _process;
	Buffer      _instream;  // -> process stdin
	Buffer      _outstream; // <- process stdout
	ScriptState _state;
};
