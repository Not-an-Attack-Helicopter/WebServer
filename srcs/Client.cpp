/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 12:43:43 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/05 12:43:44 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Client.hpp"
// #include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/HTTPRequestParser.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/templates.hpp"
#include "../incs/utils.hpp"
// #include <fstream>
// #include <sstream>
// #include <cstddef>
// #include <cstring>
// #include <fcntl.h>
#include <arpa/inet.h>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(const sockaddr_in socket, const Config::Socket* config)
	:	_state(IDLE),
		_blocked_from_receiving(false),
		_marked_for_termination(false),
		_server_addr(socket),
		_addrlen(sizeof(_remote_addr)),
		_config(config),
		_last_event(std::time(NULL)) {

	log.debug("Client Constructor called");

	std::memset(&_remote_addr, 0, _addrlen);
	_response.headers.clear();
	_response.body.temp.clear();
	_response.body.file.clear();
	_response.body.size = 0;
	_response.body.sink = NONE;

	// Create new request object in deque container
	pushRequest();
	// Create new response object in deque container
	pushResponse();

	return;

}

/*	@brief Destructor	*/
Client::~Client(void) {

	log.debug("Client Destructor called");

	if (_state == Client::RECEIVING_BODY) {
		std::string path;
		const HTTPRequest& request = *_request_queue.back();
		if (!request.body.path.empty()) {
			path = request.body.path;
			log.error("path: " + path);
			if (std::remove(path.c_str()) != 0) {
				log.error("error on deleting file");
			}
			log.info("Deleted " + path);
		}
		for (std::size_t i = 0; i < request.body.parts.size(); ++i) {
			log.error(i2a(i) + " - DING!");
			if (!request.body.parts[i].path.empty()) {
				path = request.body.parts[i].path;
				log.error("path: " + path);
				if (std::remove(path.c_str()) != 0) {
					log.error("error on deleting file");
				}
				log.info("Deleted " + path);
			}
		}
	}
	while (!_request_queue.empty()) popRequest();
	 _request_queue.clear();
	while (!_response_queue.empty()) popResponse();
	 _request_queue.clear();

	return;

}

// DEBUG BEGIN
double Client::getIdleTime(void) const {
	return (std::difftime(std::time(NULL), _last_event));
}

unsigned short int Client::getHostPort(void) const {
	sockaddr_in* addr_in = (sockaddr_in*)&_remote_addr;
	return ntohs(addr_in->sin_port);
}

const std::string Client::getHostAddress(void) const {
	char ipstr[INET_ADDRSTRLEN] = {0};
	sockaddr_in* addr_in = (sockaddr_in*)&_remote_addr;
	inet_ntop(AF_INET, &addr_in->sin_addr, ipstr, INET_ADDRSTRLEN);
	return std::string(ipstr);
}

const std::string Client::getBuffer(void) const {
	return std::string(&_instream.data[_instream.begin]);
}
// DEBUG END

const Client::State& Client::getState(void) const {
	return _state;
}

sockaddr& Client::getRemoteAddr(void) {
	return *(sockaddr*)&_remote_addr;
}

socklen_t& Client::getRemoteAddrlen(void) {
	return _addrlen;
}

const Config::Socket& Client::getConfig(void) const {
	return *_config;
}

HTTPRequest& Client::getCurrentRequest(void) {
	return *_request_queue.front();
}

HTTPRequest& Client::getRecentRequest(void) {
	return *_request_queue.back();
}

HTTPResponse& Client::getCurrentResponse(void) {
	return *_response_queue.front();
}

Buffer& Client::getIncomingData(void) {
	return _instream;
}

void Client::setState(State state) {
	_state = state;
}

bool Client::hasPendingResponse(void) const {
	return !_response_queue.empty();
}

bool Client::blockedFromReceiving(void) const {
	return _blocked_from_receiving;
}

bool Client::markedForTermination(void) const {
	return _marked_for_termination;
}

bool Client::isTimedOut(void) const {

	std::time_t timeout = 0;
	switch (_state) {
	case IDLE:
		timeout = IDLE_TIMEOUT_SECONDS;
		break;
	case RECEIVING_HEADERS:
		timeout = HEADER_TIMEOUT_SECONDS;
		break;
	case RECEIVING_BODY:
		timeout = BODY_TIMEOUT_SECONDS;
		break;
	case DISPATCHING:
		timeout = DISPATCH_TIMEOUT_SECONDS;
		break;
	case PREPARING_RESPONSE:
		timeout = DISPATCH_TIMEOUT_SECONDS;
		break;
	case AWAITING_CGI_OUTPUT:
		timeout = DISPATCH_TIMEOUT_SECONDS;
		break;
	case PENDING_RESPONSE:
		timeout = DISPATCH_TIMEOUT_SECONDS;
		break;
	case SENDING_HEADERS:
		timeout = HEADER_TIMEOUT_SECONDS;
		break;
	case SENDING_BODY:
		timeout = BODY_TIMEOUT_SECONDS;
		break;
	case CONCLUDED:
		return true;
	case REJECTED:
		timeout = REJECTED_TIMEOUT_SECONDS;
		break;
	case ERROR:
		return true;
	}

	const std::time_t now = std::time(NULL);
	return std::difftime(now, _last_event) > timeout;

}

ssize_t Client::queueIncomingData(int fd) {
	ssize_t bytes_read = _instream.fetchData(fd);
	if (bytes_read > 0) _last_event = std::time(NULL);
	return bytes_read;
}

void Client::parseIncomingData(void) {

	HTTPRequest& request = *_request_queue.back();

	if (request.parsing.state == HTTPRequest::READING_BODY &&
		_instream.data.size() == BUFFER_SIZE) {
		std::size_t buffer_size = _adjustBufferSize(request.body.size);
		_instream.data.resize(buffer_size);
	}

	while (_instream.mark < _instream.end) {

		std::size_t bytes_read = 0;
		bool has_consumed_line = false;

		try {
			has_consumed_line = parse.buffer(_instream, request);
		} catch (std::exception& e) {
			request.parsing.error_cause = INTERNAL_SERVER_ERROR;
			request.parsing.state = HTTPRequest::ERROR;
			break;
		}

		if (request.parsing.state == HTTPRequest::ERROR) {
			break;
		}

		bytes_read = request.parsing.bytes_read_count;
		if (bytes_read == std::string::npos || bytes_read == 0) {
			return;
		} else {
			_instream.mark += bytes_read;
		}

		if (has_consumed_line == true) {
			_instream.begin = _instream.mark;
		}

		if (_instream.begin == _instream.end) {

			_instream.reset();

		} else if (_instream.end == _instream.data.size()) {

			if (_instream.begin > 0) {
				_instream.compact();
			} else {
				log.error("parse error: buffer overflow");
				request.parsing.error_cause = INTERNAL_SERVER_ERROR;
				request.parsing.state = HTTPRequest::ERROR;
				break;
			}

		}

		if (request.parsing.state == HTTPRequest::RESOLVING_ROUTE) {
			break;
		}

		if (request.parsing.state == HTTPRequest::READING_BODY) {
			request.parsing.body_size += bytes_read;

			if (request.body_chunked &&
				request.parsing.body_size > request.resolved.location->client_max_body_size) {
				request.parsing.error_cause = PAYLOAD_TOO_LARGE;
				request.parsing.state = HTTPRequest::ERROR;
				break;
			}

			if ((request.parsing.chunk_state == HTTPRequest::END_OF_CHUNKS) ||
				(request.parsing.multipart_state == HTTPRequest::END_OF_PARTS) ||
				(request.parsing.body_size == request.body.size && !request.body_chunked)) {
					request.parsing.state = HTTPRequest::COMPLETE;
			}
		}

		if (request.parsing.state == HTTPRequest::COMPLETE) {

			if (request.body_chunked) {
				if (!request.requires_CGI) promoteFile(request);
				break;
			}

			if (request.parsing.body_size < request.body.size) {
				log.error("parse error: received body shorter than advertised size");
				request.parsing.state = HTTPRequest::ERROR;
				request.parsing.error_cause = BAD_REQUEST;
				break;
			}

			if (request.parsing.body_size > request.body.size) {
				log.error("parse error: received body exceeded advertised size");
				request.parsing.state = HTTPRequest::ERROR;
				request.parsing.error_cause = BAD_REQUEST;
				break;
			}

			if (!request.requires_CGI) promoteFile(request);
			break;

		}

	}

	dumpRequest(&request);

	switch (request.parsing.state) {

		case HTTPRequest::READING_REQUEST_LINE:
			setState(Client::RECEIVING_HEADERS);
			break;
		case HTTPRequest::READING_HEADERS:
			setState(Client::RECEIVING_HEADERS);
			break;
		case HTTPRequest::READING_BODY:
			setState(Client::RECEIVING_BODY);
			break;
		case HTTPRequest::RESOLVING_ROUTE:
			log.info("All HTTP request headers received");
			setState(Client::DISPATCHING);
			if (_instream.data.size() != BUFFER_SIZE) {
				_instream.data.resize(BUFFER_SIZE);
			}
			break;
		case HTTPRequest::COMPLETE:
			log.info("Full HTTP request body received");
			if (request.requires_CGI) {
				setState(Client::AWAITING_CGI_OUTPUT);
			} else {
				setState(Client::PREPARING_RESPONSE);
			}
			if (_instream.data.size() != BUFFER_SIZE) {
				_instream.data.resize(BUFFER_SIZE);
			}
			_instream.reset();
			break;
		case HTTPRequest::ERROR:
			log.warn("HTTP request parser returned error");
			setState(Client::PREPARING_RESPONSE);
			if (_instream.data.size() != BUFFER_SIZE) {
				_instream.data.resize(BUFFER_SIZE);
			}
			_instream.reset();
			break;

	}

	return;

}

void Client::queueOutgoingData(void) {

	_response.headers	<< http::V_1_1 << http::_ << _response_queue.front()->getStatusCode()
						<< http::_ << _response_queue.front()->getStatusReason() << http::CRLF;

	if (!_response_queue.front()->getHeaders().empty()) {
		std::map<std::string, std::string>::const_iterator it = _response_queue.front()->getHeaders().begin();
		while (it != _response_queue.front()->getHeaders().end()) {
			_response.headers << it->first << ": " << it->second << http::CRLF;
			// log.debug(it->first + ": " + it->second);
			++it;
		}
	}
	_response.headers << http::CRLF;

	_response.body.sink = _response_queue.front()->getBodySink();
	switch (_response.body.sink) {

	case HEAP:
		_response.body.temp << _response_queue.front()->getBody();
		_response.body.size = _response_queue.front()->getBodySize();
		break;

	case DISK:
		_response.body.file.open(_response_queue.front()->getBody().c_str(), std::ios::binary);
		if (!_response.body.file.is_open()) {
			log.error("preparing send: unable to open file");
			_response.body.sink = NONE;
			break;
		}
		_response.body.size = _response_queue.front()->getBodySize();
		break;

	case NONE:
		break;
	}

	_state = SENDING_HEADERS;
	return;
}

void Client::sendOutgoingData(int fd) {

	ssize_t bytes_sent = 0;

	switch (_state) {

	case SENDING_HEADERS:

		log.info("client_" + i2a(fd) + " state: SENDING_HEADERS");
		try {
			bytes_sent = buffNflush(_response.headers, _outstream, fd);
		} catch (std::exception& e) {
			log.error(e.what());
			_state = ERROR;
			return;
		}
		if (bytes_sent <= 0) {
			_buffNflushErrorHandler(bytes_sent, fd);
			return;
		} else {
			log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(bytes_sent));
			_last_event = std::time(NULL);
		}

		if (_response.headers.eof() && _outstream.begin == _outstream.end) {
			_clearStream(_response.headers);

			if (_response.body.sink == NONE) {
				_stateTransitionHandler(fd);

			} else {

				_state = SENDING_BODY;
				log.debug("client_" + i2a(fd) + ": state set to SENDING_BODY");
			}
		}
		break;

	case SENDING_BODY:

		log.info("client_" + i2a(fd) + " state: SENDING_BODY");
		switch (_response.body.sink) {

		case HEAP:

			try {
				bytes_sent = buffNflush(_response.body.temp, _outstream, fd);
			} catch (std::exception& e) {
				log.error(e.what());
				_state = ERROR;
				return;
			}
			if (bytes_sent <= 0) {
				_buffNflushErrorHandler(bytes_sent, fd);
				return;
			} else {
				log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(bytes_sent));
				_last_event = std::time(NULL);
			}

			if (_response.body.temp.eof() && _outstream.begin == _outstream.end) {
				log.info("client_" + i2a(fd) + ": full body/file sent");
				_clearStream(_response.body.temp);
				_stateTransitionHandler(fd);
			}
			break;

		case DISK:

			if (_outstream.data.size() == BUFFER_SIZE) {
				std::size_t buffer_size = _adjustBufferSize(_response.body.size);
				_outstream.data.resize(buffer_size);
			}

			try {
				bytes_sent = buffNflush(_response.body.file, _outstream, fd);
			} catch (std::exception& e) {
				log.error(e.what());
				_state = ERROR;
				return;
			}
			if (bytes_sent <= 0) {
				_buffNflushErrorHandler(bytes_sent, fd);
				return;
			} else {
				log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(bytes_sent));
				_last_event = std::time(NULL);
			}

			if (_response.body.file.eof() && _outstream.begin == _outstream.end) {
				log.info("client_" + i2a(fd) + ": full body/file sent");
				_clearStream(_response.body.file);
				_stateTransitionHandler(fd);
				_outstream.data.resize(BUFFER_SIZE);
			}
			break;

		default:
			break;
		}
	default:
		break;
	}
	return;
}

// Create new request object in deque container
void Client::pushRequest(void) {

	HTTPRequest* request = new HTTPRequest();
	_request_queue.push_back(request);

	return;
}

// Create new response object in deque container
void Client::pushResponse(void) {

	HTTPResponse* response = new HTTPResponse;
	_response_queue.push_back(response);

	return;
}

// Delete processed request from deque container
void Client::popRequest(void) {

	delete _request_queue.front();
	_request_queue.pop_front();

	return;
}

// Delete processed response object in deque container
void Client::popResponse(void) {

	delete _response_queue.front();
	_response_queue.pop_front();

	return;
}

void Client::blockFromReceiving(void) {
	_blocked_from_receiving = true;
	return;
}


void Client::markForTermination(void) {
	_marked_for_termination = true;
	return;
}

void Client::reset(void) {

	_state = IDLE;
	_blocked_from_receiving = false;
	_marked_for_termination = false;
	std::memset(&_remote_addr, 0, _addrlen);

	if (!_request_queue.empty()) {
		while (_request_queue.begin() != _request_queue.end()) {
			delete _request_queue.back();
			_request_queue.pop_back();
		}
		_request_queue.clear();
	}

	if (!_response_queue.empty()) {
		while (_response_queue.begin() != _response_queue.end()) {
			delete _response_queue.front();
			_response_queue.pop_front();
		}
		_response_queue.clear();
	}

	_response.headers.clear();
	_response.body.temp.clear();
	_response.body.file.clear();
	_response.body.size = 0;
	_response.body.sink = NONE;
	_instream.data.resize(BUFFER_SIZE);
	_instream.reset();
	_outstream.data.resize(BUFFER_SIZE);
	_outstream.reset();

	pushRequest();

	_last_event = std::time(NULL);
	return;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	 _state(other._state) {
	log.debug("Client Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	if (this != &other) {
		log.debug("Client Copy Assignment Operator called");
	}
	return *this;
}

std::size_t Client::_adjustBufferSize(std::size_t payload_size) {
	if (payload_size < std::size_t(5) * 1024) return 8 * 1024;
	else if (payload_size < std::size_t(50) * 1024) return 16 * 1024;
	else if (payload_size < std::size_t(500) * 1024) return 32 * 1024;
	else if (payload_size < std::size_t(5) * 1024 * 1024) return 64 * 1024;
	else if (payload_size < std::size_t(50) * 1024 * 1024) return 128 * 1024;
	else if (payload_size < std::size_t(500) * 1024 * 1024) return 192 * 1024;
	else return 256 * 1024;
}

void Client::_buffNflushErrorHandler(ssize_t bytes_sent, int fd) {
	if (bytes_sent == -1) {
		log.error("send: client_" + i2a(fd) + ": " + std::string(strerror(errno)));
	}
	_state = ERROR;
	return;
}

void Client::_stateTransitionHandler(int fd) {
	if (_blocked_from_receiving) {
		_state = REJECTED;
		log.debug("client_" + i2a(fd) + ": state set to REJECTED");
	} else if (_marked_for_termination) {
		_state = CONCLUDED;
		log.debug("client_" + i2a(fd) + ": state set to CONCLUDED");
	} else {
		_state = IDLE;
		log.debug("client_" + i2a(fd) + ": state set to IDLE");
	}
}

// Overload for std::stringstream
void Client::_clearStream(std::stringstream& stream) {
	stream.str("");
	stream.clear();
}

// Overload for std::ifstream
void Client::_clearStream(std::ifstream& stream) {
	stream.close();
}
