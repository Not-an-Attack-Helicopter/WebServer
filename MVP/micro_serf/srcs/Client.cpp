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
#include <iostream>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(void) : _addrlen(sizeof(_addr)) {
	std::cerr	<< "\e[3;93mClient Constructor called\e[0m"
				<< std::endl;
	for (size_t i = 0; i < sizeof(_buffer); ++i) {
		_buffer[i] = 0;
	}
	return;
}

/*	@brief Destructor	*/
Client::~Client(void) {
	std::cerr	<< "\e[3;93mClient Destructor called\e[0m"
				<< std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
		_buffer(""),
		_incomingData(other._incomingData),
		_outgoingData(other._outgoingData) {
	std::cerr	<< "\e[3;93mClient Copy Constructor called\e[0m"
				<< std::endl;
	for (size_t i = 0; i < sizeof(_buffer); ++i) {
		_buffer[i] = other._buffer[i];
	}
	// *this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	std::cerr	<< "\e[3;93mClient Copy Assignment Operator called\e[0m"
				<< std::endl;
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

// DEBUG
const std::string& Client::getIncomingData(void) const {
	return _incomingData;
}

// const std::string& Client::getOutgoingData(void) const {
// 	return _outgoingData;
// }

const std::string Client::getBuffer(void) const {
	const std::string ret = _buffer;
	return ret;
}
// DEBUG

sockaddr* Client::getAddrPointer(void) const {
	return (sockaddr*)&_addr;
}

socklen_t* Client::getAddrlenPointer(void) const {
	return (socklen_t*)&_addrlen;
}

void Client::queueIncomingData(size_t len){
	// std::cout << "Buffer:\n" << _buffer << "\nEnd" << std::endl;
	_incomingData.append(_buffer, len);
	return;
}

void Client::queueOutgoingData(const std::string& message) {
	_outgoingData.append(message);
	return;
}

bool Client::hasPendingData(void) const {
	return !_outgoingData.empty();
}

ssize_t Client::fillPendingData(int fd) {
	ssize_t n = recv(fd, _buffer, sizeof(_buffer), 0);
	return n;
}

ssize_t Client::flushPendingData(int fd) {
	size_t len = 0;
	std::string s = _outgoingData;
	for (std::string::iterator i = s.begin(); i != s.end(); ++i) {
		++len;
	}
	ssize_t n = send(fd, _outgoingData.c_str(), len, 0);
	_outgoingData.erase(0, n);
	// std::cout << (_outgoingData.empty() ? "Buffer empty" : "Buffer not empty") << std::endl;
	return n;
}
