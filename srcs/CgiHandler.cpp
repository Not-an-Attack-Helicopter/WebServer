#include "../incs/CgiHandler.hpp"

CgiHandler::CgiHandler(CgiProcess* process)
	: _process(process), _state(WRITING_PIPES)
{
}

CgiHandler::~CgiHandler() {
	delete _process;
}

CgiHandler::ScriptState CgiHandler::state(void) const {
	return _state;
}
