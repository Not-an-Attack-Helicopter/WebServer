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
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(void) : _addrlen(sizeof(_addr)), _last_event(std::time(NULL)) {
	log.debug("Client Constructor called");
	return;
}

/*	@brief Destructor	*/
Client::~Client(void) {
	log.debug("Client Destructor called");
	return;
}

// DEBUG
	unsigned short Client::getHostPort(void) const {
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
	const std::string ret = _buffer;
	return ret;
}

const std::string& Client::getIncomingData(void) const {
	return _incoming_data;
}

// const std::string& Client::getOutgoingData(void) const {
// 	return _outgoing_data;
// }
// DEBUG

sockaddr* Client::getAddrPointer(void) const {
	return (sockaddr*)&_addr;
}

socklen_t* Client::getAddrlenPointer(void) const {
	return (socklen_t*)&_addrlen;
}

ssize_t Client::queueIncomingData(int fd) {
	ssize_t n = recv(fd, _buffer, sizeof(_buffer) - 1, 0);
	if (n <= 0) {
		return n;
	}
	_buffer[n] = '\0';
// DEBUG
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
// DEBUG
	_incoming_data.append(_buffer, n);
	_last_event = std::time(NULL);
	return n;
}

void Client::parseIncomingData(void) {
	switch (_request.parse(_incoming_data)) {
	case PS_REQUEST_LINE:
		break;
	case PS_READING_HEADERS:
		log.info("HTTP request incomplete: awaiting more data");
		dumpRequest(_request);
		break;
	case PS_READING_BODY:
		log.info("HTTP request incomplete: awaiting more data");
		dumpRequest(_request);
		break;
	case PS_COMPLETE:
		log.info("Valid HTTP request received");
		dumpRequest(_request);
		_incoming_data.clear();
		break;
	case PS_ERROR:
		log.error("HTTP request parser returned error");
		dumpRequest(_request);
		_incoming_data.clear();
		break;
	}
}

void Client::queueOutgoingData(const std::string& message) {
	_outgoing_data.append(message);
	_last_event = std::time(NULL);
	return;
}

bool Client::hasPendingData(void) const {
	return !_outgoing_data.empty();
}

ssize_t Client::flushPendingData(int fd) {
	ssize_t n = send(fd, _outgoing_data.c_str(), _outgoing_data.size(), 0);
	if (n <= 0) {
		return n;
	}
	_outgoing_data.erase(0, static_cast<size_t>(n));
	// log.debug(_outgoing_data.empty() ? "Full flush" : "Partial flush");
	_last_event = std::time(NULL);
	return n;
}

bool Client::isTimedOut(void) const {
	// double idleTime = std::difftime(std::time(NULL), _last_event);
	// log.debug("client " + getHostAddress() + ":" + i2a(getHostPort()) + " idleTime: " + i2a(idleTime));
	// return idleTime > CONNECTION_IDLE_TIMEOUT_SECONDS;
	return std::difftime(std::time(NULL), _last_event) > CONNECTION_IDLE_TIMEOUT_SECONDS;
}

// DEBUG
double Client::getIdleTime(void) const {
	return (std::difftime(std::time(NULL), _last_event));
}
// DEBUG

void Client::reset(void) {
	_incoming_data.clear();
	_outgoing_data.clear();
	for (size_t i = 0; i < sizeof(_buffer); ++i) {
		_buffer[i] = '\0';
	}
	// HTTPRequest request;
	_request.reset();
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
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
		for (size_t i = 0; i < sizeof(_buffer); ++i) {
			_buffer[i] = other._buffer[i];
		}
		_incoming_data = other._incoming_data;
		_outgoing_data = other._outgoing_data;
	}
	return *this;
}
