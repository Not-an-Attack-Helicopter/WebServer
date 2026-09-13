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
#include <cstddef>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(const sockaddr_in socket, const Config::Socket* config)
	:	cgi_process(NULL),
		_state(IDLE),
		_blocked_from_receiving(false),
		_marked_for_termination(false),
		_config(config),
		_server_addr(socket),
		_addrlen(sizeof(_remote_addr)),
		_request(NULL),
		_response(NULL),
		_last_event(std::time(NULL)) {

	log.debug("Client Constructor called");

	std::memset(&_remote_addr, 0, _addrlen);
	_pending_response.headers.clear();
	_pending_response.body.temp.clear();
	_pending_response.body.file.clear();
	_pending_response.body.size = 0;
	_pending_response.body.sink = NONE;

	// Create new request object
	pushRequest();
	// Create new response object
	pushResponse();

	return;
}

/*	@brief Destructor	*/
Client::~Client(void) {

	log.debug("Client Destructor called");

	if (_state == Client::RECEIVING_BODY) {
		std::string path;
		const HTTPRequest& request = *_request;
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

	delete cgi_process;
	// while (!process_queue.empty()) popProcess();
	// process_queue.clear();
	// while (!_request_queue.empty()) popRequest();
	// _request_queue.clear();
	// while (!_response_queue.empty()) popResponse();
	// _response_queue.clear();
	if (_request != NULL) popRequest();
	if (_response != NULL) popResponse();

	return;
}

// DEBUG BEGIN
double Client::getIdleTime(void) const {
	const std::time_t now = std::time(NULL);
	return (std::difftime(now, _last_event));
}

unsigned short int Client::getRemotePort(void) const {
	sockaddr_in* addr_in = (sockaddr_in*)&_remote_addr;
	return ntohs(addr_in->sin_port);
}

const std::string Client::getRemoteAddress(void) const {
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
	return *_request;
}

// HTTPRequest& Client::getRecentRequest(void) {
// 	return *_request_queue.back();
// }

HTTPResponse& Client::getCurrentResponse(void) {
	return *_response;
}

Buffer& Client::getIncomingData(void) {
	return _instream;
}

void Client::setState(State state) {
	_state = state;
}

bool Client::hasPendingResponse(void) const {
	// return !_response_queue.empty();
	return _response != NULL;
}

bool Client::blockedFromReceiving(void) const {
	return _blocked_from_receiving;
}

bool Client::markedForTermination(void) const {
	return _marked_for_termination;
}

bool Client::isTimedOut(const std::time_t now) const {

	std::time_t timeout = 0;
	switch (_state) {
	case IDLE:
		timeout = IDLE_TIMEOUT_SECONDS;
		break;
	case RECEIVING_HEADERS:
		timeout = HEADER_TIMEOUT_SECONDS;
		break;
	 case RETRIEVING_SESSION:
	 timeout = PROCESSING_TIMEOUT_SECONDS;
	  break;
	case RECEIVING_BODY:
		timeout = BODY_TIMEOUT_SECONDS;
		break;
	case DISPATCHING:
		timeout = PROCESSING_TIMEOUT_SECONDS;
		break;
	case PREPARING_RESPONSE:
		timeout = PROCESSING_TIMEOUT_SECONDS;
		break;
	case AWAITING_CGI_OUTPUT:
		timeout = PROCESSING_TIMEOUT_SECONDS;
		break;
	case PENDING_RESPONSE:
		timeout = PROCESSING_TIMEOUT_SECONDS;
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

	return std::difftime(now, _last_event) > timeout;
}

ssize_t Client::queueIncomingData(int fd) {
	ssize_t bytes_read = _instream.fetchData(fd);
	if (bytes_read > 0) _last_event = std::time(NULL);
	return bytes_read;
}

void Client::parseDataFromPeer(void) {

	HTTPRequest& request = *_request;
	// CGIProcess* process = process_queue.back();

	if (request.parsing.state == HTTPRequest::READING_BODY &&
		_instream.data.size() == BUFFER_SIZE) {
		std::size_t buffer_size = _adjustBufferSize(request.body.size);
		_instream.data.resize(buffer_size);
	}

	while (_instream.mark < _instream.end) {

		bool has_consumed_line = parse.buffer(_instream, cgi_process, request);
		if (request.parsing.state == HTTPRequest::ERROR) {
			break;
		}

		std::size_t bytes_read = request.parsing.bytes_read_count;
		if (bytes_read == 0 || bytes_read == std::string::npos) {
			return;
		} else {
			_instream.mark += bytes_read;
			_last_event = std::time(NULL);
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

			if (!request.body_chunked) {

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
			setState(Client::RETRIEVING_SESSION);
			if (_instream.data.size() != BUFFER_SIZE) {
				_instream.data.resize(BUFFER_SIZE);
			}
			break;
		case HTTPRequest::COMPLETE:
			log.info("Full HTTP request body received");
			// We could set client state to AWAITING_CGI_OUTPUT
			// here, instead of having the dispatcher do it
			// if (request.requires_CGI == true) {
			// 	setState(Client::AWAITING_CGI_OUTPUT);
			// } else {}
			setState(Client::PREPARING_RESPONSE);
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

	_pending_response.headers	<< HTTP::V_1_1 << HTTP::_ << _response->getStatusCode()
						<< HTTP::_ << _response->getStatusReason() << HTTP::CRLF;

	if (!_response->getHeaders().empty()) {
		std::map<std::string, std::string>::const_iterator it = _response->getHeaders().begin();
		while (it != _response->getHeaders().end()) {
			_pending_response.headers << it->first << ": " << it->second << HTTP::CRLF;
			// log.debug(it->first + ": " + it->second);
			++it;
		}
	}
	_pending_response.headers << HTTP::CRLF;

	_pending_response.body.sink = _response->getBodySink();
	switch (_pending_response.body.sink) {

	case HEAP:
		_pending_response.body.temp << _response->getBody();
		_pending_response.body.size = _response->getBodySize();
		break;

	case DISK:
		_pending_response.body.file.open(_response->getBody().c_str(), std::ios::binary);
		if (!_pending_response.body.file.is_open()) {
			log.error("preparing send: unable to open file");
			_pending_response.body.sink = NONE;
			break;
		}
		_pending_response.body.size = _response->getBodySize();
		break;

	case NONE:
		break;
	}

	_state = SENDING_HEADERS;
	return;
}

static inline ssize_t buffNflush(std::istream& stream, Buffer& b, int fd) {

	// Fill buffer if not saturated and stream has not reached EOF
	if (!stream.eof() && b.end < b.data.size()) {
		stream.read(&b.data[b.end], b.data.size() - b.end);
		std::streamsize bytes_read = stream.gcount();
		if (bytes_read > 0) b.end += static_cast<std::size_t>(bytes_read);
	}

	// Send/write pending bytes
	ssize_t n = b.flushData(fd);
	if (n < 0) return n;

	// Everything has been sent/written; reset indices
	if (b.begin == b.end) {
		b.reset();

	// Compact buffer if needed
	} else if (b.end == b.data.size()) {

		if (b.begin > 0) {
			b.compact();
		} else {
			throw std::runtime_error("client_" + i2a(fd) + ": buffer overflow");
		}
	}

	return n;
}

void Client::sendDataToTCPPeer(int fd) {

	ssize_t bytes_sent = 0;
	std::istream* data = NULL;

	switch (_state) {

	case SENDING_HEADERS:

		log.info("client_" + i2a(fd) + " sending response headers");

		data = &_pending_response.headers;
		break;

	case SENDING_BODY:

		log.info("client_" + i2a(fd) + " sending response body");

		switch (_pending_response.body.sink) {

		case HEAP:

			data = &_pending_response.body.temp;
			break;

		case DISK:

			if (_outstream.data.size() == BUFFER_SIZE) {
				std::size_t buffer_size = _adjustBufferSize(_pending_response.body.size);
				_outstream.data.resize(buffer_size);
			}

			data = &_pending_response.body.file;
			break;

		default:
			break;
		}

	default:
		break;
	}

	try {

		bytes_sent = buffNflush(*data, _outstream, fd);

	} catch (std::exception& e) {

		log.error(e.what());
		_state = ERROR;
		log.debug("client_" + i2a(fd) + ": state set to ERROR");
		return;

	}

	if (bytes_sent <= 0) {

		if (bytes_sent == -1) {
			log.error("send: client_" + i2a(fd) + ": " + std::string(strerror(errno)));
		}
		_state = ERROR;
		log.debug("client_" + i2a(fd) + ": state set to ERROR");
		return;

	} else {

		log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(bytes_sent));
		_last_event = std::time(NULL);

	}

	if (data->eof() && _outstream.begin == _outstream.end) {

		_clearStream(*data);

		switch (_state) {

		case SENDING_HEADERS:

			if (_pending_response.body.sink == NONE) {
				log.info("client_" + i2a(fd) + ": full response sent");
				_stateTransitionHandler(fd);
			} else {
				log.info("client_" + i2a(fd) + ": all headers sent");
				_state = SENDING_BODY;
			}
			break;

		case SENDING_BODY:

			log.info("client_" + i2a(fd) + ": full body/file sent");
			_stateTransitionHandler(fd);
			if (_pending_response.body.sink == DISK && _outstream.data.size() != BUFFER_SIZE) {
				_outstream.data.resize(BUFFER_SIZE);
			}
			break;

		default:
			break;
		}
	}

	return;
}

// Create new request object
void Client::pushRequest(void) {

	// HTTPRequest* request = new HTTPRequest((sockaddr_in*)&_remote_addr, &_server_addr);
	// _request_queue.push_back(request);
	_request = new HTTPRequest((sockaddr_in*)&_remote_addr, &_server_addr);;

	return;
}

// Create new response object
void Client::pushResponse(void) {

	// HTTPResponse* response = new HTTPResponse;
	// _response_queue.push_back(response);
	_response = new HTTPResponse;

	return;
}

// Delete processed request object
void Client::popRequest(void) {

	// delete _request_queue.front();
	// _request_queue.pop_front();
	delete _request;
	_request = NULL;

	return;
}

// Delete processed response object
void Client::popResponse(void) {

	// delete _response_queue.front();
	// _response_queue.pop_front();
	delete _response;
	_response = NULL;

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

void Client::updateTimeStamp(void) {
	_last_event = std::time(NULL);
	return;
}

void Client::reset(void) {

	delete cgi_process;
	cgi_process = NULL;
	_state = IDLE;
	_blocked_from_receiving = false;
	_marked_for_termination = false;
	_config = NULL;
	std::memset(&_server_addr, 0, _addrlen);
	std::memset(&_remote_addr, 0, _addrlen);

	// while (!process_queue.empty()) popProcess();
	// process_queue.clear();
	// while (!_request_queue.empty()) popRequest();
	// _request_queue.clear();
	// while (!_response_queue.empty()) popResponse();
	// _response_queue.clear();
	if (_request !=  NULL) popRequest();
	if (_response != NULL) popResponse();

	_pending_response.headers.clear();
	_pending_response.body.temp.clear();
	_pending_response.body.file.clear();
	_pending_response.body.size = 0;
	_pending_response.body.sink = NONE;
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
	return;
}

void Client::_clearStream(std::istream& stream) {

	std::stringstream* stringstream = dynamic_cast<std::stringstream*>(&stream);
	if (stringstream != NULL) {
		stringstream->str("");
		stringstream->clear();
		return;
	}

	std::ifstream* filestream = dynamic_cast<std::ifstream*>(&stream);
	if (filestream != NULL) {
		filestream->close();
		return;
	}

}
