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
	// std::cerr	<< "\e[3;93mClient Constructor called: "
	// 			<< _id << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Constructor called\e[0m"
				<< std::endl;
	return;
}

/*	@brief Destructor	*/
Client::~Client(void) {
	// std::cerr	<< "\e[3;93mClient Destructor called: "
	// 			<< _id << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Destructor called\e[0m"
				<< std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Client::Client(const Client& other)
	:	_addr(other._addr),
		_addrlen(other._addrlen),
		// _id(other._id * 10),
		_incomingData(other._incomingData),
		_outgoingData(other._outgoingData) {
	// std::cerr	<< "\e[3;93mClient Copy Constructor called: "
	// 				<< other._id << " -> " << _id << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Copy Constructor called\e[0m"
				<< std::endl;
	// *this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Client& Client::operator = (const Client& other) {
	// std::cerr	<< "\e[3;93mClient Copy Assignment Operator called: "
	// 			<< other._id << " -> " << _id  << "\e[0m" << std::endl;
	std::cerr	<< "\e[3;93mClient Copy Assignment Operator called\e[0m"
				<< std::endl;
	if (this != &other) {
		_addr = other._addr;
		_addrlen = other._addrlen;
		_incomingData = other._incomingData;
		_outgoingData = other._outgoingData;
	}
	return *this;
}

const std::string& Client::getIncomingData(void) const {
	return (_incomingData);
}

const std::string& Client::getOutgoingData(void) const {
	return (_outgoingData);
}

void Client::queueIncomingData(char buffer[1024]) {
	_incomingData.append(buffer);
	return;
}

void Client::queueOutgoingData(const std::string& message) {
	_outgoingData.append(message);
	return;
}

bool Client::hasPendingData(void) const {
	return !_outgoingData.empty();
}

int Client::flushPendingData(int fd) {
	// std::cout << "Trying to send: " << _outgoingData << std::endl;
	ssize_t n = send(fd, _outgoingData.c_str(), sizeof(_outgoingData), 0);
	_outgoingData.erase(0, n);
	// std::cout << _outgoingData.empty() << std::endl;
	// std::cout << "-----\n" << _outgoingData << "-----\n" << std::endl;
	return n;
}

sockaddr* Client::getAddrPointer(void) const {
	return (sockaddr*)&_addr;
}

socklen_t* Client::getAddrlenPointer(void) const {
	return (socklen_t*)&_addrlen;
}
