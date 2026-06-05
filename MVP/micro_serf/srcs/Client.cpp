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

// int Client::count = 0;

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Client::Client(void) : _addrlen(sizeof(_addr))/*, _id(++count)*/ {
	// std::cerr << "\e[3;93mClient Constructor called: " << _id << "\e[0m" << std::endl;
	std::cerr << "\e[3;93mClient Constructor called\e[0m" << std::endl;
	return;
}

/*	@brief Destructor	*/
Client::~Client(void) {
	// std::cerr << "\e[3;93mClient Destructor called: " << _id << "\e[0m" << std::endl;
	std::cerr << "\e[3;93mClient Destructor called\e[0m" << std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
		// _id(other._id * 10),
		_readBuffer(other._readBuffer),
		_writeBuffer(other._writeBuffer) {
	// std::cerr	<< "\e[3;93mClient Copy Constructor called: " << other._id << " -> " << _id << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Copy Constructor called\e[0m" << std::endl;
	// *this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	// std::cerr	<< "\e[3;93mClient Copy Assignment Operator called: " << other._id << " -> " << _id  << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Copy Assignment Operator called\e[0m" << std::endl;
	if (this != &other) {
		_addr = other._addr;
		_addrlen = other._addrlen;
		_readBuffer = other._readBuffer;
		_writeBuffer = other._writeBuffer;
	}
	return *this;
}

bool Client::hasPendingWrites(void) const {
	return !_writeBuffer.empty();
}

void Client::queueResponse(const std::string& message) {
	_writeBuffer.append(message);
	return;
}

size_t Client::flush(int fd) {
	int ret = fd;
	return ret;
}

sockaddr* Client::getAddrPointer(void) const {
	return (sockaddr*)&_addr;
}

socklen_t* Client::getAddrlenPointer(void) const {
	return (socklen_t*)&_addrlen;
}
