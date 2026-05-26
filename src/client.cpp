#include "client.hpp"

// ─── Constructors / Destructor ───────────────────────────────────────────────

Client::Client(int fd, ServerConfig* server)
	: _fd(fd), _server(server), _last_active(std::time(NULL)), _parse_state(PS_READING_HEADERS)
{
}

Client::Client(const Client& other)
	: _fd(other._fd),
	  _server(other._server),
	  _recv_buf(other._recv_buf),
	  _send_buf(other._send_buf),
	  _last_active(other._last_active),
	  _parse_state(other._parse_state),
	  _request(other._request),
	  _response(other._response)
{
}

Client& Client::operator=(const Client& other)
{
	if (this != &other) {
		_fd          = other._fd;
		_server      = other._server;
		_recv_buf    = other._recv_buf;
		_send_buf    = other._send_buf;
		_last_active = other._last_active;
		_parse_state = other._parse_state;
		_request     = other._request;
		_response    = other._response;
	}
	return *this;
}

Client::~Client() {}

// ─── Accessors ───────────────────────────────────────────────────────────────

int            Client::getFd()         const { return _fd; }
ServerConfig*  Client::getServer()     const { return _server; }
time_t         Client::getLastActive() const { return _last_active; }
ParseState     Client::getParseState() const { return _parse_state; }
void           Client::setParseState(ParseState state) { _parse_state = state; }

const std::string& Client::getRecvBuf()    const { return _recv_buf; }
void               Client::clearRecvBuf()        { _recv_buf.clear(); }
void               Client::setSendBuf(const std::string& data) { _send_buf = data; }
bool               Client::isSendBufEmpty() const { return _send_buf.empty(); }

HTTPRequest&       Client::getRequest()        { return _request; }
const HTTPRequest& Client::getRequest()  const { return _request; }
HTTPResponse&      Client::getResponse()       { return _response; }

// ─── I/O ─────────────────────────────────────────────────────────────────────

bool Client::read_data()
{
	char buffer[4096];
	ssize_t bytes_read = recv(_fd, buffer, sizeof(buffer), 0);
	if (bytes_read > 0) {
		_recv_buf.append(buffer, bytes_read);
		_last_active = std::time(NULL);
		return true;
	}
	return false;
}

bool Client::write_data()
{
	if (_send_buf.empty())
		return true;
	ssize_t bytes_sent = send(_fd, _send_buf.c_str(), _send_buf.size(), MSG_NOSIGNAL);
	if (bytes_sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;    // socket not ready, retry on next epoll event
		return false;       // real error
	}
	_send_buf.erase(0, bytes_sent);
	_last_active = std::time(NULL);
	return true;
}

bool Client::is_timed_out() const
{
	return std::difftime(std::time(NULL), _last_active) > 60;
}

void Client::reset()
{
	_recv_buf.clear();
	_send_buf.clear();
	_parse_state = PS_READING_HEADERS;
	_request     = HTTPRequest();
	_response    = HTTPResponse();
	_last_active = std::time(NULL);
}

