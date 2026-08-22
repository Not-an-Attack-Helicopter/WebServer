/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 12:43:59 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/05 12:44:00 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Config.hpp"
// #include "Logger.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
// #include "templates.hpp"
#include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <string>
// #include <vector>
#include <deque>
#include <ctime>
#include <cstring>
#include <cstddef>

// struct Body {
// 	std::stringstream			temp;
// 	// bool						body_pending;
// 	std::ifstream				file;
// 	// bool						file_pending;
// 	Sink						sink;
// };

// struct OutgoingData {
// 	std::stringstream			headers;
// 	// std::stringstream			body;
// 	// std::ifstream				file;
// 	// bool						is_file;
// 	Body						body;
// };

class Client {

public:

	Client(const Config::Socket* config);
	~Client(void);

	enum State {
		IDLE,
		RECEIVING_HEADERS,
		RECEIVING_BODY,
		DISPATCHING,
		PENDING_RESPONSE,
		SENDING_HEADERS,
		SENDING_BODY,
		CONCLUDED,
		REJECTED,
		ERROR
		// SENDING_STRINGSTREAM,
		// SENDING_FILESTREAM,
		// SENDING_FILE
   };

	// enum SendResult {
	// 	PROGRESS,
	// 	DONE,
	// 	ERROR
	// };

	struct Buffer {

		std::vector<char> data;
		size_t begin;
		size_t mark;
		size_t end;

		// std::vector<char>::const_iterator begin_it = data.begin() + begin;
		// std::vector<char>::const_iterator end_it = data.begin() + end;

		std::string str(void) const;
		std::string substr(ssize_t begin) const;
		std::string substr(ssize_t begin, ssize_t end) const;

		void sstream(std::stringstream& ss) const;
		void sstream(std::stringstream& ss, ssize_t begin) const;
		void sstream(std::stringstream& ss, ssize_t begin, ssize_t end) const;

		void reset(void);
		void compact(void);

		size_t range(void) const;

		ssize_t find(const char& pin) const;
		ssize_t find(const std::string& needle) const;

	};

	struct Body {
		std::stringstream			temp;
		// bool						body_pending;
		std::ifstream				file;
		// bool						file_pending;
		size_t						size;
		Sink						sink;
	};

	struct Response {
		std::stringstream			headers;
		// std::stringstream			body;
		// std::ifstream				file;
		// bool						is_file;
		Body						body;
	};

	// template<typename StreamType> // send data
	// ssize_t							flushPendingData(int fd, StreamType& stream) {
 //
	// 	ssize_t bytes_sent = 0;
	//
	// 	if (stream.fail()) {
	// 		_bytes_read = 0;
	// 		_clearStream(stream);
	// 		log.warn("client: failbit set on stream");
	// 		switch (_state) {
	// 			case SENDING_HEADERS:
	// 				if (_response_queue.front()->getBodyType() == HTTPResponse::TEXT) {
	// 					_state = SENDING_BODY;
	// 					stream = _outgoing.body;
	// 				} else if (_response_queue.front()->getBodyType() == HTTPResponse::FILE_PATH) {
	// 					_state = SENDING_FILE;
	// 					stream = _outgoing.file;
	// 				} else {
	// 					_state = IDLE;
	// 					return -6;
	// 				}
	// 				break;
	// 			case SENDING_BODY:
	// 				_state = IDLE;
	// 				return -5;
	// 			case SENDING_FILE:
	// 				_state = IDLE;
	// 				return -4;
	// 			case IDLE:
	// 				return -3;
	// 		}
	// 	}
 //
	// 	stream.read(_buffer + _bytes_read, BUFFER_SIZE - _bytes_read);
 //
	// 	_bytes_read = stream.gcount();
	// 	if (_bytes_read == 0) { // EOF reached
	// 		_clearStream(stream);
	// 		return -2;
	// 	} else { // send chunk of bytes
	// 		bytes_sent = send(fd, _buffer, _bytes_read, 0);
	// 	}
 //
	// 	if (bytes_sent <= 0) { // error
	// 		return bytes_sent;
	// 	} else if (bytes_sent < _bytes_read) { // partial flush: shift buffer & adjust bytes_read
	// 		log.debug("client: partial flush");
	// 		std::memmove(_buffer, _buffer + bytes_sent, _bytes_read - bytes_sent);
	// 		_bytes_read -= bytes_sent;
	// 	} else { // full flush: ready for next chunk
	// 		log.debug("client: full flush");
	// 		std::memset(_buffer, 0, BUFFER_SIZE); // necessary? // TODO // DECISION REQUIRED // TODO
	// 		_bytes_read = 0;
	// 	}
 //
	// 	return bytes_sent;
	// }

// DEBUG BEGIN
	// enum AdminCommand {
	// 	STOP = 2000
	// };
	static const short				STOP = -2;
	double							getIdleTime(void) const;
	unsigned short int				getHostPort(void) const;
	const std::string				getHostAddress(void) const;
	const std::string				getBuffer(void) const;
	// const std::string&				getIncomingData(void) const;
	// void							queueOutgoingData(const std::string& message);
// DEBUG END

	// size_t							max_body_size;

	const State&					getState(void) const;

	// sockaddr*						getAddrPointer(void) const;
	sockaddr&						getAddr(void);

	// socklen_t*						getAddrlenPointer(void) const;
	socklen_t&						getAddrlen(void);

	// const Config*					getConfigPointer(void) const;
	const Config::Socket&			getConfig(void) const;

	HTTPRequest&					getCurrentRequest(void);
	HTTPRequest&					getRecentRequest(void);

	HTTPResponse&					getCurrentResponse(void);

	Buffer&							getIncomingData(void);

	// const OutgoingData*				getOutgoingData(void) const;

	// std::stringstream*				getHeaders(void);

	void							setState(State state);
	// void							setBytesRead(ssize_t bytes_read);

	// bool							hasPendingRequest(void) const;
	bool							hasPendingResponse(void) const;
	bool							hasPendingData(void) const;
	bool							blockedFromReceiving() const;
	bool							markedForTermination() const;
	bool							isTimedOut(void) const;

	// ssize_t							recvNextChunk(int fd);

	ssize_t							queueIncomingData(int fd); // receive data

	void							parseIncomingData(void); // build request
	// void							parseBody(void); // build request
	void							queueOutgoingData(void); // prepare response
	void							flushPendingData(int fd); // send data
	void							pushRequest(void);
	void							pushResponse(void);
	void							popRequest(void);
	void							popResponse(void);
	void							blockFromReceiving(void);
	void							markForTermination(void);
	void							reset(void);


private:

	Client(const Client& other);
	Client& operator = (const Client& other);

	static const time_t				IDLE_TIMEOUT_SECONDS		= 60;
	static const time_t				HEADER_TIMEOUT_SECONDS		= 10;
	static const time_t				DISPATCH_TIMEOUT_SECONDS	= 120;
	static const time_t				BODY_TIMEOUT_SECONDS		= 120;
	static const time_t				REJECTED_TIMEOUT_SECONDS	= 10;

	static const size_t				BUFFER_SIZE = 4 * 1024;

	// static const size_t				CONTENT_STREAM_BUFFER_SIZE = 4096;
	// static const size_t				LOW_LATENCY_BUFFER_SIZE = 8192;
	// static const size_t				GENERAL_PURPOSE_BUFFER_SIZE = 16384;
	// static const size_t				HIGH_THROUGHPUT_BUFFER_SIZE = 32768;
	// static const size_t				BULK_DATA_BUFFER_SIZE = 65536;

	// char							_buffer[SEND_BUFFER_SIZE];

	// char							_content_stream_buffer[CONTENT_STREAM_BUFFER_SIZE];
	// char							_low_latency_buffer[LOW_LATENCY_BUFFER_SIZE];
	// char							_general_purpose_buffer[GENERAL_PURPOSE_BUFFER_SIZE];
	// char							_high_throughput_buffer[HIGH_THROUGHPUT_BUFFER_SIZE];
	// char							_bulk_data_buffer[BULK_DATA_BUFFER_SIZE];

	State							_state;

	bool							_blocked_from_receiving;
	bool							_marked_for_termination;

	sockaddr_storage				_addr;

	socklen_t						_addrlen;

	const Config::Socket*			_config;

	std::deque<HTTPRequest*>		_request_queue;		// FIFO queue of requests to dispatch
	std::deque<HTTPResponse*>		_response_queue;	// FIFO queue of responses to send

	// std::vector<char>				_buffer;

	// ssize_t							_bytes_read;
	// ssize_t							_bytes_sent;

	// size_t							_begin;
	// size_t							_end;

	// std::string						_incoming_data;

	Buffer							_instream;
	Buffer							_outstream;

	Response						_response;

	bool							_eof_reached;

	time_t							_last_event;

	// template<typename StreamType>
	// bool							_sendNextChunk(int fd, StreamType& stream) {
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
	// 	stream.read(_buffer + _bytes_read, BUFFER_SIZE - _bytes_read);
 //
	// 	_bytes_read += stream.gcount();
	// 	log.debug("client_" + i2a(fd) + ": bytes read: " + i2a(_bytes_read));
	// 	// log.error(_buffer);
	// 	if (_bytes_read == 0) { // no more bytes in stream
	// 		_clearStream(stream);
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
	// 	// } else if (stream.eof()) { // EOF reached
	// 	// 	log.error("eofbit set on stream");
	// 	// 	_clearStream(stream);
	// 	} else {
	// 		// log.debug("sending chunk of bytes");
	// 		_bytes_sent = send(fd, _buffer, _bytes_read, 0); // send chunk of bytes
	// 		log.debug("client_" + i2a(fd) + ": bytes sent: " + i2a(_bytes_sent));
	// 	}
 //
	// 	if (_bytes_sent <= 0) { // error
	// 		return false;
	// 	} else if (_bytes_sent < _bytes_read) { // partial flush
	// 	// if (0 < bytes_sent && bytes_sent < _bytes_read) { // partial flush
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
	// 		_clearStream(stream);
	// 		return true;
	// 	} else {
	// 		return false;
	// 	}
	// }

	bool							_sendNextChunk(int fd, std::istream& stream);

	size_t							_adjustBufferSize(size_t payload_size);

	void							_clearStream(std::stringstream& stream);
	void							_clearStream(std::ifstream& stream);

};

// #include "templates.tpp"

#endif

// void								handleRequest(void); // TEST
// void								buildCSSResponse(void); // TEST
// void								buildResponse(void); // TEST

// stream >> _buffer; ???
// static_cast<const void*>(_buffer)

// if (stream.fail()) {return -1;}
// int nextState = static_cast<int>(_client_state) + 1;
// if (nextState < static_cast<int>(IDLE)) {
// 	_client_state = (ClientState)nextState;
// } else {
// 	_client_state = IDLE;
// }

// If no bytes read yet, read a chunk of bytes
// if (_bytes_read == 0) {

// const std::string&			getOutgoingData(void) const;
// Dispatcher					_handler;
// Dispatcher&					_handler;			// Shared stateless handler
// template<typename T>
// void push();
// template<typename T>
// void pop();
// template<typename T, typename Q>
// void						push(Q& queue) {
// 	T* message = new T;
// 	queue,push_back(message);
// }
// template<typename T, typename Q>
// void						pop(Q& queue) {
// 	delete queue.front();
// 	queue.pop_front();
// }

// template<typename StreamType>
// bool							_sendNextChunk(int fd, StreamType& stream) {
// 	_bytes_sent = 0;
// 	// if (_eof_reached && _bytes_read == 0) {
// 	// 	log.error("Nothing left to send");
// 	// 	return false; // nothing left to send
// 	// }
// 	// StreamType stream = _outgoing.body;
// 	// If we haven't read yet, read a chunk
// 	if (_bytes_read == 0) {
// 		stream.read(_buffer, BUFFER_SIZE);
// 		// log.error("Inside _buffer:");
// 		// log.notice(_buffer);
// 		_bytes_read = stream.gcount();
// 		log.error("bytes read: " + i2a(_bytes_read));
// 		_total_read += _bytes_read;
// 		log.error("total read: " + i2a(_total_read));
// 		if (_bytes_read == 0) {
// 			_eof_reached = true;
// 			_clearStream(stream);
// 			return false;
// 		}
// 		if (stream.eof()) {
// 			_eof_reached = true;
// 			_clearStream(stream);
// 			// return false;
// 		}
// 	}
// 	// Send what we have
// 	// printf("SOCKET %d: About to send %ld bytes:\n%.*s\n",
// 	// 	   fd, _bytes_read, static_cast<int>(_bytes_read), _buffer);
// 	if (_bytes_read > 0) {
// 		_bytes_sent = send(fd, _buffer, _bytes_read, 0);
// 	}
// 	if (_bytes_sent < 0) {
// 		// perror("send");
// 		return false;
// 	}
// 	if (_bytes_sent < _bytes_read) {
// 		// Partial send: shift buffer and adjust bytes_read
// 		log.error("partial flush");
// 		std::memmove(_buffer, _buffer + _bytes_sent, _bytes_read - _bytes_sent);
// 		_bytes_read -= _bytes_sent;
// 	} else {
// 		// Full send: ready for next chunk
// 		log.error("full flush");
// 		// std::memset(_buffer, 0, BUFFER_SIZE);
// 		// clearStream(stream);
// 		_bytes_read = 0;
// 	}
// 	// log.error("!_eof_reached || _bytes_read > 0 ?");
// 	// log.error((!_eof_reached || _bytes_read > 0) ? "true" : "false");
// 	// log.error((!_eof_reached || _bytes_read > 0) ? "have more" : "have no more");
// 	return !_eof_reached || _bytes_read > 0;
// }

// switch (_state) {
// case SENDING_HEADERS:
// 	stream = _outgoing.headers;
// 	break;
// case SENDING_BODY:
// 	stream = _outgoing.body;
// 	break;
// case SENDING_FILE:
// 	stream = _outgoing.file;
// 	break;
// case IDLE:
// 	_bytes_read = 0;
// 	return -7;
// }
