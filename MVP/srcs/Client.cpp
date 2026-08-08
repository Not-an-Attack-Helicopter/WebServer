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
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/Parser.hpp"
#include "../incs/utils.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/templates.hpp"
#include <cstddef>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
// #include <fstream>
#include <sstream>
#include <cstring>
// #include <vector>
// #include <ios>

// template<>
// void Client::push<HTTPRequest>() {
// 	HTTPRequest* request = new HTTPRequest;
// 	_request_queue.push_back(request);
// }
//
// template<>
// void Client::push<HTTPResponse>() {
// 	HTTPResponse* response = new HTTPResponse;
// 	_response_queue.push_back(response);
// }
//
// template<>
// void Client::pop<HTTPRequest>() {
// 	delete _request_queue.front();
// 	_request_queue.pop_front();
// }
//
// template<>
// void Client::pop<HTTPResponse>() {
// 	delete _response_queue.front();
// 	_response_queue.pop_front();
// }

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(const Config::Socket* config)
	:	keepAlive(true),
		_buffer(BUFFER_SIZE),
		_state(IDLE),
		// _body_sink(HTTPResponse::UNDEFINED),
		_bytes_read(0),
		_bytes_sent(0),
		_begin(0),
		_end(0),
		_eof_reached(false),
		_addrlen(sizeof(_addr)),
		_config(config),
		_last_event(std::time(NULL)) {

	log.debug("Client Constructor called");

	// _buffer.resize(BUFFER_SIZE);
	std::memset(&_buffer[0], 0, _buffer.size());
	// std::memset(_buffer, 0, SEND_BUFFER_SIZE);
	std::memset(&_addr, 0, _addrlen);
	// std::memset(static_cast<void*>(&_outgoing_data), 0, sizeof(_outgoing_data));
	_incoming_data.clear();
	_outgoing_data.headers.clear();
	_outgoing_data.body.temp.clear();
	_outgoing_data.body.file.clear();
	_outgoing_data.body.size = 0;
	_outgoing_data.body.sink = DISK;

	// Create new request object in deque container
	// HTTPRequest* request = new HTTPRequest();
	// _request_queue.push_back(request);
	// push<HTTPRequest>();
	pushRequest();

	// Create new response object in deque container
	// HTTPResponse* response = new HTTPResponse;
	// _response_queue.push_back(response);

	// if (ss.good()) {
	// 	log.warn("goodbit is set");
	// }
	// if (ss.eof()) {
	// 	log.warn("eofbit is set");
	// }
	// if (ss.bad()) {
	// 	log.warn("badbit is set");
	// }
	// if (ss.fail()) {
	// 	log.warn("failbit is set");
	// }

	return;

}

/*	@brief Deconstructor	*/
Client::~Client(void) {

	log.debug("Client Deconstructor called");

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
	sockaddr_in* addr_in = (sockaddr_in*)&_addr;
	return ntohs(addr_in->sin_port);
}

const std::string Client::getHostAddress(void) const {
	char ipstr[INET_ADDRSTRLEN] = {0};
	sockaddr_in* addr_in = (sockaddr_in*)&_addr;
	inet_ntop(AF_INET, &addr_in->sin_addr, ipstr, INET_ADDRSTRLEN);
	return std::string(ipstr);
}

const std::string Client::getBuffer(void) const {
	return std::string(&_buffer[0]);
}

const std::string& Client::getIncomingData(void) const {
	return _incoming_data;
}

// void Client::queueOutgoingData(const std::string& message) {
//
// 	// HTTPResponse* response = new HTTPResponse;
// 	// response->setStatus(666, "Prepare To Die");
// 	// response->setHeader("Server", "MyServer/1.0");
// 	// // size_t i = -1;
// 	// // while (++i < _config->server_names.size()) {
// 	// // for (size_t i = 0; i < _config->server_names.size(); ++i) {
// 	// // 	response->setHeader("Server", _config->server_names[i]);
// 	// // }
// 	// response->setBody(message, "text/html");
// 	// _outgoing_data.append(response->serialize());
// 	// delete response;
// 	// std::ostringstream oss;
// 	// oss << 666;
// 	// std::string response = "HTTP/1.1 " + oss.str() + " Unknown Status\r\n";
// 	// oss << message.length();
// 	// response += "Content-Length: " + oss.str() + "\r\n";
// 	// response += "Content-Type: text/html\r\n";
// 	// response += "\r\n<html><body><h1>" + message + "</h1></body></html>\r\n";
// 	// std::ostringstream response;
// 	// _outgoing_data.append(response.str());
// 	_outgoing_data.body.on_heap		<< tag::DOC << tag::HTML << tag::BODY << tag::H1 << message
// 									<< tag::_H1 << tag::_BODY << tag::_HTML << http::CRLF;
// 	_outgoing_data.headers	<< http::HTTP_1_1 << http::_ << 666 << http::_ << "Unknown Status" << http::CRLF
// 							<< "Content-Length:" << http::_ << _outgoing_data.body.on_heap.str().size() << http::CRLF
// 							<< "Content-Type: text/html" << http::CRLF
// 							<< http::CRLF;
// 	_state = SENDING_HEADERS;
// 	_last_event = std::time(NULL);
// 	return;
//
// }
// DEBUG END

const Client::State& Client::getState(void) const {
	return _state;
}

// sockaddr* Client::getAddrPointer(void) const {
// 	return (sockaddr*)&_addr;
// }

sockaddr& Client::getAddr(void) {
	return *(sockaddr*)&_addr;
}

// socklen_t* Client::getAddrlenPointer(void) const {
// 	return (socklen_t*)&_addrlen;
// }

socklen_t& Client::getAddrlen(void) {
	return _addrlen;
}

// const Config* Client::getConfigPointer(void) const {
// 	return _config;
// }

const Config::Socket& Client::getConfig(void) const {
	return *_config;
}

const HTTPRequest& Client::getCurrentRequest(void) const {
	return *_request_queue.front();
}

HTTPResponse& Client::getCurrentResponse(void) {
	return *_response_queue.front();
}

// const Client::OutgoingData* Client::getOutgoingData(void) const {
// 	return &_outgoing;
// }

// std::stringstream* Client::getHeaders(void) {
// 	return &_outgoing.headers;
// }

// void Client::setBytesRead(ssize_t bytes_read) {
// 	_bytes_read = bytes_read;
// }

// bool Client::hasPendingRequest(void) const {
//
// 	// size_t i = -1;
// 	// 	while (++i < _request_queue.size()) {
//
// 	if (!_request_queue.empty()) {
// 		for (size_t i = 0; i < _request_queue.size(); ++i) {
// 			if (_request_queue[i]->getState() == COMPLETE) {
// 				return true;
// 			}
// 		}
// 	}
// 	return false;
// }

bool Client::hasPendingResponse(void) const {

	return !_response_queue.empty();
}

bool Client::hasPendingData(void) const {
	// return _hasPendingData;
	// return !_outgoing_data.empty();
	// return (_state != IDLE && _state != ERROR);
	return (_state == SENDING_HEADERS || _state == SENDING_BODY);
}

ssize_t Client::queueIncomingData(int fd) {

	// std::memset(_buffer, 0, BUFFER_SIZE);
	ssize_t n = recv(fd, &_buffer[0], _buffer.size(), 0);

	if (n <= 0) {
		return n;
	}

	// _buffer[n] = '\0'; // extra precaution
// DEBUG BEGIN
	// Interpret the first 4 bytes as an admin command.
	std::string cmd(&_buffer[0], (n < 4 ? static_cast<size_t>(n) : static_cast<size_t>(4)));
	// std::string cmd = _buffer;
	// if (cmd.size() > 4) {
	// 	cmd.erase(4);
	// }
	if (cmd == "STOP") {
		return STOP;
	}
// DEBUG END
	_incoming_data.append(&_buffer[0], n);
	_last_event = std::time(NULL);

	return n;

}

void Client::parseIncomingData(void) {

	// hrp->append(_incoming_data);
	// _incoming_data.clear();
	// if (!hrp->isComplete())
	// 	return;

	// _incoming_data = hrp->buffer;
	// // hrp->buffer.clear();
	// delete hrp;
	// hrp = new HTTPRequestParser;

	// _incoming_data = "POST / HTTP/1.1\nHost: x\nContent-Length: 6\nUser-Agent: Mozilla\n\nQWERTYPOST / HTTP/1.1\r\nHost: x\r\nContent-Length: 6\r\nUser-Agent: Mozilla\r\n\r\nQWERTY";
	// log.error("Data:");
	// log.notice(_incoming_data);
	// log.error("Size: " + i2a(_incoming_data.size()));

	// HTTPRequest*	request;
	// HTTPResponse*	response;

	while (!_incoming_data.empty()) {

		// switch (_request_queue.back()->parse(_incoming_data)) {
		switch (parse.incomingData(_incoming_data, _request_queue.back())) {

		case HTTPRequest::READING_REQUEST_LINE:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->parsing.bytes_read_count);

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case HTTPRequest::READING_HEADERS:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->parsing.bytes_read_count);

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case HTTPRequest::READING_BODY:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->parsing.bytes_read_count);

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case HTTPRequest::COMPLETE:

			log.info("Valid HTTP request received");
			dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->parsing.bytes_read_count);

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			pushRequest();
			// memset(_buffer, 0, BUFFER_SIZE); // necessary? // TODO // DECISION REQUIRED // TODO

			// handleRequest(); // TEST

			break;

		case HTTPRequest::ERROR:

			log.error("HTTP request parser returned error");
			dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->parsing.bytes_read_count);

			log.debug("Current data in buffer (client):\n");
			log.notice(_incoming_data);

			pushRequest();
			// memset(_buffer, 0, BUFFER_SIZE); // necessary? // TODO // DECISION REQUIRED // TODO
			// _request_queue.back()->reset(); // reset last request object in vector container

			break;

		}

	}

	return;

}

// TEST BEGIN
void Client::queueOutgoingData(void) {

	// delete _request_queue.front();
	// _request_queue.pop_front();

	// if (!_response_queue.empty()) {
	// if (hasPendingResponse()) {
	// 	std::string response = _response_queue.front()->serialize();
	// 	// _outgoing_data.append(response);
	// 	_outgoing.headers << response;
	// 	_outgoing.headers_pending = true;
	// } else {
	// 	log.warn("No response in queue");
	// }

	// _outgoing_data.append(_response_queue.front()->serialize());

	// // Create new response object in deque container
	// HTTPResponse* response = new HTTPResponse;
	// _response_queue.push_back(response);

	// delete _response_queue.front();
	// _response_queue.pop_front();

	_outgoing_data.headers	<< http::V_1_1 << http::_ << _response_queue.front()->getStatusCode()
							<< http::_ << _response_queue.front()->getStatusReason() << http::CRLF;

	if (!_response_queue.front()->getHeaders().empty()) {
		std::map<std::string, std::string>::const_iterator it = _response_queue.front()->getHeaders().begin();
		while (it != _response_queue.front()->getHeaders().end()) {
			_outgoing_data.headers << it->first << ": " << it->second << http::CRLF;
			// log.debug(it->first + ": " + it->second);
			++it;
		}
	}
	_outgoing_data.headers << http::CRLF;

	// if (_response_queue.front()->getBodySink() == HTTPResponse::HEAP) {
		// _body_sink = HTTPResponse::TEXT;
		// _outgoing_data.body.sink = HTTPResponse::HEAP;
	// } else if (_response_queue.front()->getBodySink() == HTTPResponse::FILE) {
		// _body_sink = HTTPResponse::FILE_PATH;
		// _outgoing_data.body.sink = HTTPResponse::FILE;

	_outgoing_data.body.sink = _response_queue.front()->getBodySink();
	// if (_outgoing_data.body.sink == HEAP) {
	switch (_outgoing_data.body.sink) {
	case HEAP:
		// log.debug("preparing send: body data read from heap");
		_outgoing_data.body.temp << _response_queue.front()->getBody();
		break;
	// } else if (_outgoing_data.body.sink == DISK) {
	case DISK:
		// log.debug("preparing send: body data read from file");
		_outgoing_data.body.file.open(_response_queue.front()->getBody().c_str(), std::ios::binary);
		if (!_outgoing_data.body.file.is_open()) {
			log.error("preparing send: unable to open file");
			return;
		}
		_outgoing_data.body.size = _response_queue.front()->getContentLength();
		break;
	}

	_state = SENDING_HEADERS;
	_last_event = std::time(NULL);
	return;

}

void Client::flushPendingData(int fd) {

	// bool fullySent;
	// std::istream& body = _outgoing_data.body.sink == DISK ?
	// static_cast<std::istream&>(_outgoing_data.body.file) : static_cast<std::istream&>(_outgoing_data.body.temp);

	switch (_state) {

		case SENDING_HEADERS:
			log.info("client_" + i2a(fd) + " state: SENDING_HEADERS");
			// fullySent = _sendNextChunk(fd, _outgoing_data.headers);
			// if (fullySent) {
			if (_sendNextChunk(fd, _outgoing_data.headers)) {
				_clearStream(_outgoing_data.headers);
				log.info("client_" + i2a(fd) + ": all headers sent");
				_state = SENDING_BODY;
				log.debug("client_" + i2a(fd) + ": state set to SENDING_BODY");
			}
			break;

		case SENDING_BODY:
			log.info("client_" + i2a(fd) + " state: SENDING_BODY");
			// fullySent = _sendNextChunk(fd, body);
			switch (_outgoing_data.body.sink) {

				case HEAP:
					// fullySent = _sendNextChunk(fd, _outgoing_data.body.temp);
					// if (fullySent) {
					if (_sendNextChunk(fd, _outgoing_data.body.temp)) {
						_clearStream(_outgoing_data.body.temp);
						log.info("client_" + i2a(fd) + ": full body/file sent");
						_state = IDLE;
						log.debug("client_" + i2a(fd) + ": state set to IDLE");
					}
					break;

				case DISK:
					// fullySent = _sendNextChunk(fd, _outgoing_data.body.file);
					// if (fullySent) {
					_buffer.resize(_adjustBufferSize());
					if (_sendNextChunk(fd, _outgoing_data.body.file)) {
						// _buffer.clear();
						_buffer.resize(BUFFER_SIZE);
						_clearStream(_outgoing_data.body.file);
						log.info("client_" + i2a(fd) + ": full body/file sent");
						_state = IDLE;
						log.debug("client_" + i2a(fd) + ": state set to IDLE");
					}
					break;

			}

				default:
					break;

	}

	return;

}
// if (_outgoing_data.body.sink == HEAP) {
// 	eof_reached = _sendNextChunk(fd, _outgoing_data.body.temp);
// } else if (_outgoing_data.body.sink == DISK) {
// 	eof_reached = _sendNextChunk(fd, _outgoing_data.body.file);
// }
// 	if (fullySent) {
// _clearStream(_outgoing_data.headers);
// 		log.info("client_" + i2a(fd) + ": full body/file sent");
// 		_state = IDLE;
// 		log.debug("client_" + i2a(fd) + ": state set to IDLE");
// 	}
// switch (_body_sink) {
// case HTTPResponse::FILE_PATH:
// 	return _sendNextChunk(fd, _outgoing_data.file);
// 	// if (bytes_sent == 0) {
// 	// 	log.error("full body/file sent");
// 	// 	_state = IDLE;
// 	// 	log.error("client state set to IDLE from SENDING_FILE");
// 	// }
// 	break;
// case HTTPResponse::TEXT:
// 	return _sendNextChunk(fd, _outgoing_data.body);
// 	// if (bytes_sent == 0) {
// 	// 	log.error("full body sent");
// 	// 	_state = IDLE;
// 	// 	log.error("client state set to IDLE from SENDING_BODY");
// 	// }
// break;
// case HTTPResponse::UNDEFINED:
// _state = IDLE;
// log.error("body type unknown");
// }
// case SENDING_FILE:
// 	log.error("sending file");
// 	bytes_sent = _sendNextChunk(fd, _outgoing_data.file);
// if (_sendNextChunk(fd, _outgoing_data.file)) {
// 	_state = IDLE;
// 	log.error("client state set to IDLE from SENDING_FILE");
// 	}
// break;
// case ERROR:
// 	log.error("client_" + i2a(fd) + " state: ERROR");
// 	// _bytes_read = 0;
// 	// // return 0;
// 	// _bytes_sent = 0;
// 	break;
// case IDLE:
// 	log.error("client_" + i2a(fd) + " state: IDLE");
// 	// _bytes_read = 0;
// 	// // return 0;
// 	// _bytes_sent = 0;
// 	break;
// 	default:
// 		break;
// 	}
//
// 	return _bytes_sent;;
//
// }
// TEST END

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

}

// Delete processed response object in deque container
void Client::popResponse(void) {

	delete _response_queue.front();
	_response_queue.pop_front();

}

void Client::reset(void) {

	keepAlive = true;
	_state = IDLE;
	_bytes_read = 0;
	_bytes_sent = 0;
	_begin = 0;
	_end = 0;
	_eof_reached = false;
	std::memset(&_buffer[0], 0, _buffer.size());
	// std::memset(_buffer, 0, SEND_BUFFER_SIZE);
	std::memset(&_addr, 0, _addrlen);
	// std::memset(static_cast<void*>(&_outgoing_data), 0, sizeof(_outgoing_data));

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

	_incoming_data.clear();
	_outgoing_data.headers.clear();
	_outgoing_data.body.temp.clear();
	_outgoing_data.body.file.clear();
	_outgoing_data.body.size = 0;
	_outgoing_data.body.sink = DISK;

	// HTTPRequest* request = new HTTPRequest();
	// _request_queue.push_back(request);
	// push<HTTPRequest>();
	pushRequest();

	_last_event = std::time(NULL);

	return;

}

bool Client::isTimedOut(void) const {
	// double idleTime = std::difftime(std::time(NULL), _last_event);
	// log.debug("client " + getHostAddress() + ":" + i2a(getHostPort()) + " idleTime: " + i2a(idleTime));
	// return idleTime > CONNECTION_IDLE_TIMEOUT_SECONDS;
	return std::difftime(std::time(NULL), _last_event) > CONNECTION_IDLE_TIMEOUT_SECONDS;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	keepAlive(other.keepAlive) {
		// _buffer(other._buffer),
		// _state(other._state),
		// // _body_sink(other._body_sink),
		// _bytes_read(other._bytes_read),
		// _bytes_sent(other._bytes_sent),
		// _begin(other._begin),
		// _end(other._end),
		// _eof_reached(other._eof_reached),
		// _addr(other._addr),
		// _addrlen(other._addrlen),
		// _config(other._config),
		// _request_queue(other._request_queue),
		// _response_queue(other._response_queue),
		// _incoming_data(other._incoming_data),
		// // _outgoing_data(other._outgoing_data),
		// _last_event(other._last_event) {
	log.debug("Client Copy Constructor called");
	// for (size_t i = 0; i < sizeof(_buffer); ++i) {
	// for (size_t i = 0; i < _buffer.size(); ++i) {
	// for (size_t i = 0; i < BUFFER_SIZE; ++i) {
	// 	_buffer[i] = other._buffer[i];
	// }
	// *this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	log.debug("Client Copy Assignment Operator called");
	if (this != &other) {
		keepAlive = other.keepAlive;
		// _buffer = other._buffer;
		// _state = other._state;
		// // _body_sink = other._body_sink;
		// _bytes_read = other._bytes_read;
		// _bytes_sent = other._bytes_sent;
		// _begin = other._begin;
		// _end = other._end;
		// _eof_reached = other._eof_reached;
		// _addr = other._addr;
		// _addrlen = other._addrlen;
		// _config = other._config;
		// _incoming_data = other._incoming_data;
		// // _outgoing_data = other._outgoing_data;
		// _request_queue = other._request_queue;
		// _response_queue = other._response_queue;
		// _last_event = other._last_event;
		// // for (size_t i = 0; i < sizeof(_buffer); ++i) {
		// // for (size_t i = 0; i < _buffer.size(); ++i) {
		// // for (size_t i = 0; i < BUFFER_SIZE; ++i) {
		// // 	_buffer[i] = other._buffer[i];
		// // }
	}
	return *this;
}

bool Client::_sendNextChunk(int fd, std::istream& stream) {

	std::ostringstream oss;
	oss << "stream.tellg(): " << i2a(stream.tellg())
		<< std::endl
		<< "before read: "
		<< "good=" << stream.good()
		<< " eof=" << stream.eof()
		<< " fail=" << stream.fail()
		<< " bad=" << stream.bad()
		<< std::endl;
	// log.error(oss.str());
	// oss.clear();

	// Fill buffer if there is space in the ring buffer and stream has not reached EOF
	if (!_eof_reached && _end < _buffer.size()) {
		stream.read(&_buffer[_end], _buffer.size() - _end);
		oss << "after read: "
			<< "gcount=" << stream.gcount()
			<< " eof=" << stream.eof()
			<< " fail=" << stream.fail()
			<< std::endl;
		// log.error(oss.str());
		// oss.clear();
		_bytes_read += stream.gcount();
		log.debug("client_" + i2a(fd) + ": bytes read: " + i2a(_bytes_read));

		if (_bytes_read > 0) _end += static_cast<size_t>(_bytes_read);
		oss << "bytes_read=" << _bytes_read
			<< " begin=" << _begin
			<< " end=" << _end
			<< std::endl;

		if (stream.eof()) {
			 _eof_reached = true;
			// switch (_outgoing_data.body.sink) {
			// case HEAP:
			// 	// _clearStream(dynamic_cast<std::stringstream&>(stream));
			// 	_clearStream(_outgoing_data.body.temp);
			// 	break;
			// case DISK:
			// 	// _clearStream(dynamic_cast<std::ifstream&>(stream));
			// 	_clearStream(_outgoing_data.body.file);
			// 	break;
			// }
		}
	}

	// Send pending bytes
	if (_begin < _end) {
		_bytes_sent = send(fd, &_buffer[_begin], _end - _begin, 0);
		switch (_bytes_sent) {
			case -1:
				log.error("send: client_" + i2a(fd) + ": " + std::string(strerror(errno)));
				_state = ERROR;
				return false;
			case 0:
				_state = ERROR;
				return false;
			default:
				log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(_bytes_sent));
				_begin += static_cast<size_t>(_bytes_sent);
		}
	}
	log.debug(oss.str());
	oss.clear();

	// Everything has been sent; reset buffer
	if (_begin == _end) {
		_begin = 0;
		_bytes_read = 0;
		_bytes_sent = 0;
		_end = 0;

	// Roll-over/compact buffer if needed
	} else if (_end == _buffer.size()) {
		std::memmove(&_buffer[0], &_buffer[_begin], _end - _begin);
		_end -= _begin;
		_begin = 0;
	}

	// Done only when stream ended AND buffer is empty
	if (_eof_reached && _begin == _end) {
		_eof_reached = false;
		return true;
	}

	return false;

}

size_t Client::_adjustBufferSize(void) {
	if (_outgoing_data.body.size < std::size_t(5) * 1024) return 8 * 1024;
	else if (_outgoing_data.body.size < std::size_t(50) * 1024) return 16 * 1024;
	else if (_outgoing_data.body.size < std::size_t(500) * 1024) return 32 * 1024;
	else if (_outgoing_data.body.size < std::size_t(5) * 1024 * 1024) return 64 * 1024;
	else if (_outgoing_data.body.size < std::size_t(50) * 1024 * 1024) return 128 * 1024;
	else if (_outgoing_data.body.size < std::size_t(500) * 1024 * 1024) return 192 * 1024;
	else return 256 * 1024;
}


// bool Client::_sendNextChunk(int fd, std::istream& stream) {
//
// 	// ssize_t bytes_sent;
//
// 	// if (stream.good()) {
// 	// 	log.error("good bit set! STREAM IS GOOD!!!!!!");
// 	// }
//
// 	// if (stream.fail()) {
// 	// 	_bytes_read = 0;
// 	// 	_clearStream(stream);
// 	// 	log.warn("client: failbit set on stream");
// 	// 	switch (_state) {
// 	// 	case SENDING_HEADERS:
// 	// 		if (_body_sink == HTTPResponse::TEXT) {
// 	// 			_state = SENDING_BODY;
// 	// 			log.error("client state set to SENDING_BODY");
// 	// 			// stream = _outgoing_data.body;
// 	// 			// _sendNextChunk(fd, _outgoing_data.body);
// 	// 			return 0;
// 	// 		} else if (_body_sink == HTTPResponse::FILE_PATH) {
// 	// 			_state = SENDING_FILE;
// 	// 			log.error("client state set to SENDING_FILE");
// 	// 			// stream = _outgoing_data.file;
// 	// 			// _sendNextChunk(fd, _outgoing_data.file);
// 	// 			return 0;
// 	// 		} else {
// 	// 			_state = IDLE;
// 	// 			log.error("body type unknown");
// 	// 			return 0;
// 	// 		}
// 	// 		break;
// 	// 	case SENDING_BODY:
// 	// 		_state = IDLE;
// 	// 		log.error("client state set to IDLE from SENDING_BODY");
// 	// 		return 0;
// 	// 	case SENDING_FILE:
// 	// 		_state = IDLE;
// 	// 		log.error("client state set to IDLE from SENDING_FILE");
// 	// 		return 0;
// 	// 	case IDLE:
// 	// 		log.error("client state stays on IDLE");
// 	// 		return 0;
// 	// 	}
// 	// }
//
// 	// log.error("bytes read: " + i2a(_bytes_read));
// 	// log.error(_buffer + _bytes_read);
// 	log.error("stream.tellg(): " + i2a(stream.tellg()));
// 	std::ostringstream oss;
// 	oss << "before read: "
// 		<< "good=" << stream.good()
// 		<< " eof=" << stream.eof()
// 		<< " fail=" << stream.fail()
// 		<< " bad=" << stream.bad()
// 		<< std::endl;
// 	log.error(oss.str());
// 	oss.clear();
// 	stream.read(_buffer + _bytes_read, BUFFER_SIZE - _bytes_read);
// 	oss << "after read: "
// 		<< "gcount=" << stream.gcount()
// 		<< " eof=" << stream.eof()
// 		<< " fail=" << stream.fail()
// 		<< std::endl;
// 	log.error(oss.str());
// 	oss.clear();
// 	_bytes_read += stream.gcount();
// 	oss << "bytes_read=" << _bytes_read
// 		<< " begin=" << _begin
// 		<< " end=" << _end
// 		<< std::endl;
// 	log.error(oss.str());
// 	oss.clear();
// 	log.debug("client_" + i2a(fd) + ": bytes read: " + i2a(_bytes_read));
// 	// log.error(_buffer);
// 	if (_bytes_read == 0) { // no more bytes in stream
// 		// _clearStream(stream);
// 		switch (_outgoing_data.body.sink) {
// 		case HEAP:
// 			// _clearStream(dynamic_cast<std::stringstream&>(stream));
// 			_clearStream(_outgoing_data.body.temp);
// 			break;
// 		case DISK:
// 			// _clearStream(dynamic_cast<std::ifstream&>(stream));
// 			_clearStream(_outgoing_data.body.file);
// 			break;
// 		}
// 		// if (_state == SENDING_HEADERS) {
// 		// 	if (_body_sink == HTTPResponse::TEXT) {
// 		// 		_state = SENDING_BODY;
// 		// 		log.error("client state set to SENDING_BODY");
// 		// 	} else if (_body_sink == HTTPResponse::FILE_PATH) {
// 		// 		_state = SENDING_FILE;
// 		// 		log.error("client state set to SENDING_FILE");
// 		// 	} else {
// 		// 		_state = IDLE;
// 		// 		log.error("body type unknown");
// 		// 	}
// 		// } else {
// 		// 	_state = IDLE;
// 		// 	log.error("client state set to IDLE");
// 		// }
// 		return true;
// 		// } else if (stream.eof()) { // EOF reached
// 		// 	log.error("eofbit set on stream");
// 		// 	_clearStream(stream);
// 	} else {
// 		// log.debug("sending chunk of bytes");
// 		_bytes_sent = send(fd, _buffer, _bytes_read, 0); // send chunk of bytes
// 		log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(_bytes_sent));
// 	}
//
// 	if (_bytes_sent <= 0) { // error
// 		return false;
// 	} else if (_bytes_sent < _bytes_read) { // partial flush
// 		// if (0 < bytes_sent && bytes_sent < _bytes_read) { // partial flush
// 		log.debug("client_" + i2a(fd) + ": partial flush"); // shift buffer & adjust bytes_read
// 		std::memmove(_buffer, _buffer + _bytes_sent, _bytes_read - _bytes_sent);
// 		_bytes_read -= _bytes_sent;
// 	} else { // full flush: ready for next chunk
// 		log.debug("client_" + i2a(fd) + ": full flush");
// 		std::memset(_buffer, 0, BUFFER_SIZE); // necessary? // TODO // DECISION REQUIRED // TODO
// 		_bytes_read = 0;
// 	}
// 	if (stream.eof()) { // EOF reached
// 		// log.debug("eofbit set on stream");
// 		// _clearStream(stream);
// 		switch (_outgoing_data.body.sink) {
// 		case HEAP:
// 			// _clearStream(dynamic_cast<std::stringstream&>(stream));
// 			_clearStream(_outgoing_data.body.temp);
// 			break;
// 		case DISK:
// 			// _clearStream(dynamic_cast<std::ifstream&>(stream));
// 			_clearStream(_outgoing_data.body.file);
// 			break;
// 		}
// 		return true;
// 	} else {
// 		return false;
// 	}
// }

// Overload for std::stringstream
void Client::_clearStream(std::stringstream& stream) {
	stream.str("");
	stream.clear();
}

// Overload for std::ifstream
void Client::_clearStream(std::ifstream& stream) {
	// stream.clear();
	// stream.seekg(0, std::ios::beg);
	stream.close();
}

// if (!_request_queue.empty()) {
// 	// while (_request_queue.begin() != _request_queue.end()) {
// 	// 	delete _request_queue.front();
// 	// 	// *_request_queue.begin() = NULL;
// 	// 	_request_queue.erase(_request_queue.begin());
// 	// }
// 	while (_request_queue.begin() != _request_queue.end()) {
// 		// delete _request_queue.back();
// 		// _request_queue.pop_back();
// 		// pop<HTTPRequest>();
// 		popRequest();
// 	}
// 	_request_queue.clear();
// }

// if (!_response_queue.empty()) {
// 	// while (_response_queue.begin() != _response_queue.end()) {
// 	// 	delete _response_queue.front();
// 	// 	// *_response_queue.begin() = NULL;
// 	// 	_response_queue.erase(_response_queue.begin());
// 	// }
// 	while (_response_queue.begin() != _response_queue.end()) {
// 		// delete _response_queue.front();
// 		// _response_queue.pop_front();
// 		// pop<HTTPResponse>();
// 		popResponse();
// 	}
// 	_response_queue.clear();
// }

// void Client::processNextRequest() {
// 	if (!_request_queue.empty()) {
// 		HTTPResponse* response;
// 		_response_queue.push_back(response);
// 		_handler.handle(*_request_queue.front(), *_response_queue.back(), *_config);
// 	}
// }

// void Client::processRequests() {
// 	while (!_request_queue.empty()) {
// 		HTTPRequest* request = _request_queue.front();
// 		HTTPResponse* response = new HTTPResponse();
// 		_handler.handle(*request, *response, *_config);
// 		_response_queue.push_back(response);
// 		_request_queue.erase(_request_queue.begin());
// 		delete request;
// 	}
// }

// void Client::sendResponse(HTTPResponse* res) {
// 	// Write to socket...
// 	_response_queue.erase(_response_queue.begin());
// 	delete res;
// }

// void Client::handleRequest(void) {
// 	// log.error(_request_queue.at(_request_queue.size() - 1)->getPath());
// 	if (_request_queue.back()->getPath() == "/pages/error/style.css") {
// 		buildCSSResponse();
// 	} else {
// 		buildResponse();
// 		// HTTPResponse* response = new HTTPResponse;
// 		// response->setStatus(405);
// 		// response->setHeader("Server", "MyServer/1.0");
// 		// std::string body =	"<html><body><h1>" + i2a(response->getStatusCode()) +
// 		// 					": " + response->getStatusReason() + "</h1></body></html>";
// 		// response->setBody(body, "text/html");
// 		// _response_queue.push_back(response);
// 	}
// 	delete _request_queue.back();
// 	_request_queue.pop_back();
// 	return;

// }

// void Client::buildCSSResponse(void) {
// 	std::string response_file = "pages/error/style.css";
// 	std::ifstream file(response_file.c_str());
// 	if (!file.is_open()) {
// 		throw std::runtime_error("could not open file: " + response_file);
// 	}
// 	HTTPResponse* response = new HTTPResponse;
// 	response->setStatus(200);
// 	response->setHeader("Server", "MyServer/1.0");
// 	// std::string body;
// 	// std::string line;
// 	// while (std::getline(file, line)) {
// 	// 	body += line + "\n";
// 	// }
// 	std::string body((std::istreambuf_iterator<char>(file)),
// 					 std::istreambuf_iterator<char>());
// 	response->setBody(body, "text/css");
// 	_response_queue.push_back(response);
// 	return;
// }

// void Client::buildResponse(void) {
// 	std::string response_file = "pages/error/400.html";
// 	std::ifstream file(response_file.c_str());
// 	if (!file.is_open()) {
// 		throw std::runtime_error("could not open file: " + response_file);
// 	}
// 	// Create new response object in deque container
// 	HTTPResponse* response = new HTTPResponse;
// 	response->setStatus(400);
// 	response->setHeader("Server", "MyServer/1.0");
// 	// Option 1:
// 	std::string body((std::istreambuf_iterator<char>(file)),
// 					 std::istreambuf_iterator<char>());
// 	// How it works:
// 	//  - Reads directly from the file's internal buffer without character-by-character parsing
// 	//  - The constructor receives two iterators and copies data in bulk chunks
// 	//  - Single allocation (or a few reallocations as the string grows)
// 	//  - No intermediate buffering — data flows directly from file buffer to string buffer
// 	// Time complexity: O(n) where n = file size
// 	// Space complexity: O(n) for the result string
// 	// Memory allocations: Typically 1–3 (depending on string growth strategy)
// 	// Option 2:
// 	// std::stringstream buffer;
// 	// buffer << file.rdbuf();
// 	// std::string body = buffer.str();
// 	// How it works:
// 	//  - file.rdbuf() returns the file's streambuf pointer
// 	//  - operator<< on stringstream calls sputn(), which copies the buffer contents
// 	//  - str() returns a copy of the internal string (one extra copy operation)
// 	// Time complexity: O(n) + cost of one extra string copy
// 	// Space complexity: O(n) for stringstream + O(n) for the result string = 2n total
// 	// Memory allocations: Multiple allocations in stringstream + one copy in str()
// 	// Extra overhead: Option 2 creates an intermediate stringstream object and copies its contents to the result string. This is less efficient than Option 1.
// 	// Option "Stoopid":
// 	// std::string body;
// 	// std::string line;
// 	// while (std::getline(file, line)) {
// 	// 	body += line + "\n";
// 	// }
// 	// How it works:
// 	//  - Reads line-by-line via getline()
// 	//  - Each += triggers a string concatenation (potentially O(n) per iteration)
// 	//  - Creates temporary line + "\n" string for each line
// 	// Time complexity: O(n²) in worst case if string doesn't pre-allocate
// 	// Space complexity: O(n) for result + O(m) per line for temporaries
// 	// Memory allocations: Many — one per line, plus reallocations as body grows
// 	response->setBody(body, "text/html");
// 	_response_queue.push_back(response);
// 	return;
// }

// size_t to_send = 0;
// size_t content_length;
// if (hasPendingData) {
// 	content_length = _outgoing_data.str().size();
// } else if (hasPendingFile) {
// 	std::ifstream _outgoing_file(_response_queue.front()->getFilePath(), std::ios::binary);
// 	// content_length = _response_queue.front()->getContentLength();
// 	_outgoing_file.seekg(0, std::ios::end);
// 	content_length = _outgoing_file.tellg();
// 	_outgoing_file.seekg(0, std::ios::beg);
// }
// if (_offset < content_length) {
// 	to_send = std::min(BUFFER_SIZE, content_length - _offset);
// 	_outgoing_file.seekg(_offset);
// 	bytes_sent = send(fd, &_outgoing_file, to_send, 0);
// 	_offset += bytes_sent;
// }
// if (_offset >= content_length) {
// 	if (hasPendingData) hasPendingData = false;
// 	if (hasPendingFile) hasPendingFile = false;
// 	log.debug("Client: Full flush");
// } else {
// 	log.debug("Client: Partial flush");
// }
// _last_event = std::time(NULL);
// return bytes_sent;

// ssize_t Client::flushPendingData(int fd) {
// ssize_t n = send(fd, _outgoing_data.c_str(), _outgoing_data.size(), 0);
// if (n < 0) {
// 	return n;
// }
// if (n == 0) {
// 	log.warn("Client: No data has been sent");
// }
// _outgoing_data.erase(0, static_cast<size_t>(n));
// log.debug(_outgoing_data.empty() ? "Client: Full flush" : "Client: Partial flush");
// if (_outgoing_data.empty()) {
// 	_hasPendingData = false;
// }
// ssize_t bytes_sent = 0;
// if (hasPendingData) {
// 	bytes_sent = send(fd, _outgoing_data.str().c_str(), _outgoing_data.str().size(), 0);
// 	hasPendingData = false;
// 	return bytes_sent;
// }
// if (hasPendingFile) {
// 	if (_eof_reached && _bytes_read == 0) {
// 		hasPendingFile = false;
// 		return bytes_sent; // Nothing left to send
// 	}
// 	// If we haven't read yet, read a chunk
// 	if (_bytes_read == 0) {
// 		_outgoing_file.read(_outbuffer, BUFFER_SIZE);
// 		_bytes_read = _outgoing_file.gcount();
// 		if (_bytes_read == 0) {
// 			_eof_reached = true;
// 			hasPendingFile = false;
// 			return bytes_sent;
// 		}
// 		if (_outgoing_file.eof()) {
// 			_eof_reached = true;
// 		}
// 	}
// 	// Send what we have
// 	bytes_sent = send(fd, _outbuffer, _bytes_read, 0);
// 	if (bytes_sent < 0) {
// 		perror("send");
// 		hasPendingFile = false;
// 	}
// 	// Partial send: shift buffer and adjust _bytes_read
// 	if (static_cast<size_t>(bytes_sent) < _bytes_read) {
// 		std::memmove(_outbuffer, _outbuffer + bytes_sent, _bytes_read - bytes_sent);
// 		_bytes_read -= bytes_sent;
// 	// Full send: ready for next chunk
// 	} else {
// 		_bytes_read = 0;
// 	}
// 	return bytes_sent;
// ssize_t bytes_sent = 0;
// _outgoing_data	<< http::HTTP_1_1 << http::_ << _response_queue.front()->getStatusCode()
// << http::_ << _response_queue.front()->getStatusReason() << http::CRLF;
// if (!_response_queue.front()->getHeaders().empty()) {
// 	std::map<std::string, std::string>::const_iterator it = _response_queue.front()->getHeaders().begin();
// 	while (it != _response_queue.front()->getHeaders().end()) {
// 		// response += it->first + ": " + it->second + "\r\n";
// 		_outgoing_data << it->first << ": " << it->second << http::CRLF;
// 		// log.error(it->first + ": " + it->second);
// 		++it;
// 	}
// }
// hasPendingData = true;
// if (_response_queue.front()->getBodyType() == HTTPResponse::TEXT) {
// _outgoing_data	<< http::CRLF << tag::HTML << tag::BODY << tag::H1
// << _response_queue.front()->getBody()
// << tag::_H1 << tag::_BODY << tag::_HTML << http::CRLF;
// log.error(_outgoing_data.str());
// if (hasPendingData) {
// 	while (_outgoing_data.read(_outbuffer, BUFFER_SIZE) || _outgoing_data.gcount() > 0) {
// 		log.error(_outbuffer);
// 		bytes_sent = send(fd, _outbuffer, _outgoing_data.gcount(), 0);
// 		// log.error(i2a(bytes_sent));
// 		if (bytes_sent < 0) {
// 			break;
// 		}
// 	}
// 	hasPendingData = false;
// // } else if (_response_queue.front()->getBodyType() == HTTPResponse::FILE_PATH) {
// 	// log.error(_response_queue.front()->getFilePath());
// 	// _outgoing_file = std::ifstream(_response_queue.front()->getFilePath().c_str(), std::ios::binary);
// 	// std::ifstream _outgoing_file(_response_queue.front()->getFilePath().c_str(), std::ios::binary);
// } if (hasPendingFile) {
// 	while (_outgoing_file.read(_outbuffer, BUFFER_SIZE) || _outgoing_file.gcount() > 0) {
// 		log.error(_outbuffer);
// 		bytes_sent = send(fd, _outbuffer, _outgoing_file.gcount(), 0);
// 		// log.error(i2a(bytes_sent));
// 		if (bytes_sent < 0) {
// 			break;
// 		}
// 	}
// 	_outgoing_file.close();
// 	hasPendingFile = false;
// }
// ssize_t bytes_sent = 0;
// ssize_t bytes_read = 0;
// if (hasPendingData) {
// 	log.error("Pending Data");
// 	_outgoing_data->read(_outbuffer, BUFFER_SIZE);
// 	bytes_read = _outgoing_data->gcount();
// 	log.error(i2a(bytes_read));
// 	log.error(_outbuffer);
// } else if (hasPendingFile) {
// 	log.error("Pending File");
// 	log.error(_response_queue.front()->getFilePath().c_str());
// 	_outgoing_file = new std::ifstream(_response_queue.front()->getFilePath().c_str(), std::ios::binary);
// 	if (!_outgoing_file->is_open()) {
// 		log.error("file not open");
// 	}
// 	_outgoing_file->read(_outbuffer, BUFFER_SIZE);
// 	bytes_read = _outgoing_file->gcount();
// 	log.error(i2a(bytes_read));
// 	log.error(_outbuffer);
// }
// if (bytes_read > 0) {
// 	bytes_sent = send(fd, _outbuffer, bytes_read, 0);
// 	log.error(i2a(bytes_read) + "==" + i2a(bytes_sent));
// }
// if (static_cast<size_t>(bytes_read) < BUFFER_SIZE) {
// 	if (hasPendingData) hasPendingData = false;
// 	if (hasPendingFile) hasPendingFile = false;
// 	log.debug("Client: Full flush");
// } else {
// 	log.debug("Client: Partial flush");
// }
// 	_last_event = std::time(NULL);
// 	return bytes_sent;
// }

// ssize_t Client::flushPendingData(int fd) {
//
// 	if (_outgoing.headers_pending) {
// 		// _bytes_sent = send(fd, _outgoing.headers.str().c_str(), _outgoing.headers.str().size(), 0);
// 		// _outgoing.headers_pending = false;
// 		// std::memset(_buffer, 0, BUFFER_SIZE);
// 		log.error("start sending headers");
// 		_outgoing.headers_pending = _sendNextChunk(fd, _outgoing.headers);
// 		log.error("bytes sent: " + i2a(_bytes_sent));
// 		_total_sent += _bytes_sent;
// 		log.error("total sent: " + i2a(_total_sent));
// 		log.error(_outgoing.headers_pending ? "headers pending" : "no headers pending");
// 		if (!_outgoing.headers_pending) {
// 			_bytes_read = 0;
// 			_total_read = 0;
// 			// _bytes_sent = 0;
// 			_total_sent = 0;
// 			_eof_reached = false;
// 		}
// 		// return _bytes_sent;
// 	}
//
// 	else if (_outgoing.body_pending) {
// 		// _bytes_sent = send(fd, _outgoing.body.str().c_str(), _outgoing.body.str().size(), 0);
// 		// _outgoing.body_pending = false;
// 		log.error("start sending body");
// 		_outgoing.body_pending = _sendNextChunk(fd, _outgoing.body);
// 		log.error("bytes sent: " + i2a(_bytes_sent));
// 		_total_sent += _bytes_sent;
// 		log.error("total sent: " + i2a(_total_sent));
// 		log.error(_outgoing.body_pending ? "body pending" : "no body pending");
// 		if (!_outgoing.body_pending) {
// 			_bytes_read = 0;
// 			_total_read = 0;
// 			// _bytes_sent = 0;
// 			_total_sent = 0;
// 			_eof_reached = false;
// 		}
// 		// return _bytes_sent;
// 	}
//
// 	else if (_outgoing.file_pending) {
// 		// char out[BULK_DATA_BUFFER_SIZE];
// 		// _outgoing.file.read(out, BULK_DATA_BUFFER_SIZE);
// 		// _bytes_sent = send(fd, out, _outgoing.file.gcount(), 0);
// 		// _outgoing.file_pending = false;
// 		log.error("start sending file");
// 		_outgoing.file_pending = _sendNextChunk(fd, _outgoing.file);
// 		log.error("bytes sent: " + i2a(_bytes_sent));
// 		_total_sent += _bytes_sent;
// 		log.error("total sent: " + i2a(_total_sent));
// 		log.error(_outgoing.file_pending ? "file pending" : "no file pending");
// 		if (!_outgoing.file_pending) {
// 			_bytes_read = 0;
// 			_total_read = 0;
// 			// _bytes_sent = 0;
// 			_eof_reached = false;
// 			_outgoing.file.close();
// 		}
// 		// return _bytes_sent;
// 	}
//
// 	_last_event = std::time(NULL);
//
// 	return _bytes_sent;
//
// }

// std::stringstream ss;
// ss.good() ? log.warn("goodbit: 1") : log.warn("goodbit: 0");
// ss.eof() ? log.warn("eofbit: 1") : log.warn("eofbit: 0");
// ss.bad() ? log.warn("badbit: 1") : log.warn("badbit: 0");
// ss.fail() ? log.warn("failbit: 1") : log.warn("failbit: 0");
// log.error("reading stringstream");
// ss.read(_buffer, BUFFER_SIZE);
// ss.good() ? log.warn("goodbit: 1") : log.warn("goodbit: 0");
// ss.eof() ? log.warn("eofbit: 1") : log.warn("eofbit: 0");
// ss.bad() ? log.warn("badbit: 1") : log.warn("badbit: 0");
// ss.fail() ? log.warn("failbit: 1") : log.warn("failbit: 0");
// log.error("reading stringstream again");
// ss.read(_buffer, BUFFER_SIZE);
// ss.good() ? log.warn("goodbit: 1") : log.warn("goodbit: 0");
// ss.eof() ? log.warn("eofbit: 1") : log.warn("eofbit: 0");
// ss.bad() ? log.warn("badbit: 1") : log.warn("badbit: 0");
// ss.fail() ? log.warn("failbit: 1") : log.warn("failbit: 0");
