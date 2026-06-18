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
Client::Client(void) : _addrlen(sizeof(_addr)), _lastEvent(std::time(NULL)) {
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
	return _incomingData;
}

// const std::string& Client::getOutgoingData(void) const {
// 	return _outgoingData;
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
	_incomingData.append(_buffer, n);
	_lastEvent = std::time(NULL);
	return n;
}

void Client::parseIncomingData(void) {
	switch (request.parse(_incomingData)) {
	case PS_COMPLETE:
		log.notice("Valid HTTP request received");
		log.debug("State:\t\t" + i2a(request.getState()));
		log.debug("Method:\t\t" + request.getMethod());
		log.debug("Path:\t\t" + request.getPath());
		log.debug("Query:\t\t" + request.getQuery());
		log.debug("Header:\t\t" + request.getHeader("content-type"));
		log.debug("Body:\t\t" + request.getBody());
		log.debug("Content-Length:\t" + i2a(request.getContentLength()) + "\n");
		_incomingData.clear();
		break;
	case PS_ERROR:
		log.error("HTTP request parser returned error");
		log.debug("State:\t\t" + i2a(request.getState()));
		log.debug("Method:\t\t" + request.getMethod());
		log.debug("Path:\t\t" + request.getPath());
		log.debug("Query:\t\t" + request.getQuery());
		log.debug("Header:\t\t" + request.getHeader("content-type"));
		log.debug("Body:\t\t" + request.getBody());
		log.debug("Content-Length:\t" + i2a(request.getContentLength()) + "\n");
		reset();
		break;
	}
}

void Client::queueOutgoingData(const std::string& message) {
	_outgoingData.append(message);
	_lastEvent = std::time(NULL);
	return;
}

bool Client::hasPendingData(void) const {
	return !_outgoingData.empty();
}

ssize_t Client::flushPendingData(int fd) {
	ssize_t n = send(fd, _outgoingData.c_str(), _outgoingData.size(), 0);
	if (n <= 0) {
		return n;
	}
	_outgoingData.erase(0, static_cast<size_t>(n));
	// log.debug(_outgoingData.empty() ? "Full flush" : "Partial flush");
	_lastEvent = std::time(NULL);
	return n;
}

bool Client::isTimedOut(void) const {
	// double idleTime = std::difftime(std::time(NULL), _lastEvent);
	// log.debug("client " + getHostAddress() + ":" + i2a(getHostPort()) + " idleTime: " + i2a(idleTime));
	// return idleTime > CONNECTION_IDLE_TIMEOUT_SECONDS;
	return std::difftime(std::time(NULL), _lastEvent) > CONNECTION_IDLE_TIMEOUT_SECONDS;
}

// DEBUG
double Client::getIdleTime(void) const {
	return (std::difftime(std::time(NULL), _lastEvent));
}
// DEBUG

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
		_incomingData(other._incomingData),
		_outgoingData(other._outgoingData) {
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
		_incomingData = other._incomingData;
		_outgoingData = other._outgoingData;
	}
	return *this;
}
