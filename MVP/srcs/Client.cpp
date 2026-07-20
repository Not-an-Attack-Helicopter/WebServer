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
#include "../incs/utils.hpp"
// #include "../incs/templates.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <fstream>
#include <cstring>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(const Config* config)
	:	_addrlen(sizeof(_addr)),
		_config(config),
		_last_event(std::time(NULL)),
		_close_after_response(false) {

	log.debug("Client Constructor called");

	// Create new request object in deque container
	// HTTPRequest* request = new HTTPRequest();
	// _request_queue.push_back(request);
	pushRequest();

	// Create new response object in deque container
	// HTTPResponse* response = new HTTPResponse;
	// _response_queue.push_back(response);

	return;

}

/*	@brief Deconstructor	*/
Client::~Client(void) {

	log.debug("Client Deconstructor called");

	if (!_request_queue.empty()) {
		// while (_request_queue.begin() != _request_queue.end()) {
		// 	delete _request_queue.front();
		// 	// *_request_queue.begin() = NULL;
		// 	_request_queue.erase(_request_queue.begin());
		// }
		while (_request_queue.begin() != _request_queue.end()) {
			delete _request_queue.back();
			_request_queue.pop_back();
		}
		_request_queue.clear();
	}

	if (!_response_queue.empty()) {
		// while (_response_queue.begin() != _response_queue.end()) {
		// 	delete _response_queue.front();
		// 	// *_response_queue.begin() = NULL;
		// 	_response_queue.erase(_response_queue.begin());
		// }
		while (_response_queue.begin() != _response_queue.end()) {
			delete _response_queue.front();
			_response_queue.pop_front();
		}
		_response_queue.clear();
	}

	return;

}

// DEBUG BEGIN
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
	return std::string(_buffer);
}

const std::string& Client::getIncomingData(void) const {
	return _incoming_data;
}

// const std::string& Client::getOutgoingData(void) const {
// 	return _outgoing_data;
// }
// DEBUG END

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

const Config& Client::getConfig(void) const {
	return *_config;
}

const HTTPRequest& Client::getCurrentRequest(void) const {
	return *_request_queue.front();
}

HTTPResponse& Client::getCurrentResponse(void) {
	return *_response_queue.front();
}

// bool Client::hasPendingRequest(void) const {
//
// 	// size_t i = -1;
// 	// 	while (++i < _request_queue.size()) {
//
// 	if (!_request_queue.empty()) {
// 		for (size_t i = 0; i < _request_queue.size(); ++i) {
// 			if (_request_queue[i]->getState() == PS_COMPLETE) {
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
	return !_outgoing_data.empty();
}

bool Client::shouldCloseAfterResponse(void) const {
	return _close_after_response;
}

ssize_t Client::queueIncomingData(int fd) {

	ssize_t n = recv(fd, _buffer, sizeof(_buffer) - 1, 0);

	if (n <= 0) {
		return n;
	}

	_buffer[n] = '\0'; // Extra precaution
// DEBUG BEGIN
	// Interpret the first 4 bytes as an admin command.
	// Fine, but I hate having a ternary inside of a string declaration.
	// std::string cmd(_buffer, (n < 4 ? (size_t)n : (size_t)4));
	std::string cmd = _buffer;
	if (cmd.size() > 4) {
		cmd.erase(4);
	}
	if (cmd == "STOP") {
		return STOP;
	}
// DEBUG END
	_incoming_data.append(_buffer, n);
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

		switch (_request_queue.back()->parse(_incoming_data)) {

		case PS_READING_REQUEST_LINE:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->getBytesRead());

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case PS_READING_HEADERS:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->getBytesRead());

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case PS_READING_BODY:

			// log.info("HTTP request incomplete: awaiting more data");
			// dumpRequest(_request_queue.back());
			if (_request_queue.back()->getContentLength() > _config->client_max_body_size) {
				pushResponse();
				_response_queue.back()->setStatus(413, "Payload Too Large");
				_response_queue.back()->setHeader("Server", "MyServer/1.0");
				_response_queue.back()->setHeader("Connection", "close");
				_response_queue.back()->setBody("Payload Too Large\n", "text/plain");
				_close_after_response = true;
				_incoming_data.clear();
				_request_queue.back()->reset();
				break;
			}

			_incoming_data.erase(0, _request_queue.back()->getBytesRead());

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			break;

		case PS_COMPLETE:

			log.info("Valid HTTP request received");
			dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->getBytesRead());

			// log.debug("Current data in buffer (client):\n");
			// log.notice(_incoming_data);

			// TODO // DECISION REQUIRED // TODO
			// // Create new response object in deque container
			// pushResponse();
			// Create new request object in deque container
			pushRequest();

			// handleRequest(); // TEST

			break;

		case PS_ERROR:

			log.error("HTTP request parser returned error");
			dumpRequest(_request_queue.back());

			_incoming_data.erase(0, _request_queue.back()->getBytesRead());

			log.debug("Current data in buffer (client):\n");
			log.notice(_incoming_data);

			_request_queue.back()->reset(); // reset last request object in vector container

			break;

		}

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

}

// Delete processed response object in deque container
void Client::popResponse(void) {

	delete _response_queue.front();
	_response_queue.pop_front();

}

// DEBUG BEGIN

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
//
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
//
// 	delete _request_queue.back();
// 	_request_queue.pop_back();
//
// 	return;
//
// }
//
// void Client::buildCSSResponse(void) {
//
// 	std::string response_file = "pages/error/style.css";
// 	std::ifstream file(response_file.c_str());
// 	if (!file.is_open()) {
// 		throw std::runtime_error("could not open file: " + response_file);
// 	}
//
// 	HTTPResponse* response = new HTTPResponse;
//
// 	response->setStatus(200);
// 	response->setHeader("Server", "MyServer/1.0");
//
// 	// std::string body;
// 	// std::string line;
// 	// while (std::getline(file, line)) {
// 	// 	body += line + "\n";
// 	// }
//
// 	std::string body((std::istreambuf_iterator<char>(file)),
// 					 std::istreambuf_iterator<char>());
//
// 	response->setBody(body, "text/css");
//
// 	_response_queue.push_back(response);
//
// 	return;
// }
//
// void Client::buildResponse(void) {
//
// 	std::string response_file = "pages/error/400.html";
// 	std::ifstream file(response_file.c_str());
// 	if (!file.is_open()) {
// 		throw std::runtime_error("could not open file: " + response_file);
// 	}
//
// 	// Create new response object in deque container
// 	HTTPResponse* response = new HTTPResponse;
//
// 	response->setStatus(400);
// 	response->setHeader("Server", "MyServer/1.0");
//
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
//
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
//
// 	// Extra overhead: Option 2 creates an intermediate stringstream object and copies its contents to the result string. This is less efficient than Option 1.
//
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
//
// 	response->setBody(body, "text/html");
//
// 	_response_queue.push_back(response);
//
// 	return;
//
// }
// DEBUG END

// DEBUG BEGIN
void Client::queueOutgoingData(const std::string& message) {

	HTTPResponse* response = new HTTPResponse;

	response->setStatus(666);
	// response->setHeader("Server", "MyServer/1.0");

	// size_t i = -1;
	// while (++i < _config->server_names.size()) {
	for (size_t i = 0; i < _config->server_names.size(); ++i) {
		response->setHeader("Server", _config->server_names[i]);
	}
	response->setBody(message, "text/html");

	_outgoing_data.append(response->serialize());

	delete response;

	_last_event = std::time(NULL);

	return;

}
// DEBUG END

// TEST BEGIN
void Client::queueOutgoingData(void) {

	// delete _request_queue.front();
	// _request_queue.pop_front();

	// if (!_response_queue.empty()) {
	// if (hasPendingResponse()) {
	// 	std::string response = _response_queue.front()->serialize();
	// 	_outgoing_data.append(response);
	// } else {
	// 	log.warn("No response in queue");
	// }

	_outgoing_data.append(_response_queue.front()->serialize());

	// // Create new response object in deque container
	// HTTPResponse* response = new HTTPResponse;
	// _response_queue.push_back(response);

	// delete _response_queue.front();
	// _response_queue.pop_front();

	_last_event = std::time(NULL);

	return;

}
// TEST END

ssize_t Client::flushPendingData(int fd) {

	ssize_t n = send(fd, _outgoing_data.c_str(), _outgoing_data.size(), 0);
	if (n < 0) {
		return n;
	}
	if (n == 0) {
		log.warn("Client: No data has been sent");
	}

	_outgoing_data.erase(0, static_cast<size_t>(n));
	log.debug(_outgoing_data.empty() ? "Client: Full flush" : "Client: Partial flush");
	_last_event = std::time(NULL);

	return n;

}

bool Client::isTimedOut(void) const {
	// double idleTime = std::difftime(std::time(NULL), _last_event);
	// log.debug("client " + getHostAddress() + ":" + i2a(getHostPort()) + " idleTime: " + i2a(idleTime));
	// return idleTime > CONNECTION_IDLE_TIMEOUT_SECONDS;
	return std::difftime(std::time(NULL), _last_event) > CONNECTION_IDLE_TIMEOUT_SECONDS;
}

// DEBUG BEGIN
double Client::getIdleTime(void) const {
	return (std::difftime(std::time(NULL), _last_event));
}
// DEBUG END

void Client::reset(void) {

	_incoming_data.clear();
	_outgoing_data.clear();

	for (size_t i = 0; i < sizeof(_buffer); ++i) {
		_buffer[i] = '\0';
	}

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

	pushRequest();
	// HTTPRequest* request = new HTTPRequest();
	// _request_queue.push_back(request);

	return;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
		_config(other._config),
		_incoming_data(other._incoming_data),
		_outgoing_data(other._outgoing_data) {
	log.debug("Client Copy Constructor called");
	for (size_t i = 0; i < sizeof(_buffer); ++i) {
		_buffer[i] = other._buffer[i];
	}
	// *this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	log.debug("Client Copy Assignment Operator called");
	if (this != &other) {
		_addr = other._addr;
		_addrlen = other._addrlen;
		_config = other._config;
		for (size_t i = 0; i < sizeof(_buffer); ++i) {
			_buffer[i] = other._buffer[i];
		}
		_incoming_data = other._incoming_data;
		_outgoing_data = other._outgoing_data;
	}
	return *this;
}
