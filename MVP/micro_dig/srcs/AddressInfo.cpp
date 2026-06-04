/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AddressInfo.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 21:18:36 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/02 21:18:38 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/AddressInfo.hpp"
#include <arpa/inet.h>
#include <iostream>

int AddressInfo::_status = 0;

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Destructor	*/
AddressInfo::~AddressInfo(void) {
	std::cerr << "\e[3;93mAddressInfo Destructor called\e[0m" << std::endl;
	freeaddrinfo(_pai);
	return ;
}

void AddressInfo::gai(void) {
	struct addrinfo*	head = NULL;
	_req.ai_family = AF_UNSPEC;
	_req.ai_socktype = SOCK_STREAM;
	_status = getaddrinfo(_hostname.c_str(), 0, &_req, &_pai);
	if (_status) {
		throw AddressInfo::GetAddressInfoException();
	}
	std::cout << "IP addresses for " << _hostname << ":" << std::endl;
	head = _pai;
	while (head) {
		if (head->ai_family == AF_INET) {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*) head->ai_addr;
			inet_ntop(AF_INET, &(ipv4->sin_addr), _buff, LEN);
			std::cout << "IPv4: " << _buff << std::endl;
		} else {
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) head->ai_addr;
			inet_ntop(AF_INET6, &(ipv6->sin6_addr), _buff6, LEN6);
			std::cout << "IPv6: " << _buff6 << std::endl;
		}
		head = head->ai_next;
	}
	return ;
}

const char* AddressInfo::GetAddressInfoException::what(void) const throw () {
	return (gai_strerror(_status));
}

AddressInfo& AddressInfo::getInstance(const std::string& hostname) {
	static AddressInfo _instance(hostname);
	return (_instance);
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

AddressInfo* AddressInfo::_instance = NULL;

/*	@brief Constructor	*/
AddressInfo::AddressInfo(const std::string& hostname) : _hostname(hostname) {
	std::cerr << "\e[3;93mAddressInfo Constructor called\e[0m" << std::endl;
	_pai = NULL;
	_req.ai_flags = 0;
	_req.ai_family = 0;
	_req.ai_socktype = 0;
	_req.ai_protocol = 0;
	_req.ai_addrlen = 0;
	_req.ai_addr = NULL;
	for (int i = 0; i < LEN; ++i) {
		_buff[i] = 0;
	}
	for (int i = 0; i < LEN6; ++i) {
		_buff6[i] = 0;
	}
	return ;
}

// /*	@brief Copy Constructor	*/
// AddressInfo::AddressInfo(const AddressInfo& other) : _hostname(other._hostname) {
// 	std::cerr	<< "\e[3;93mAddressInfo Copy Constructor called: "
// 				<< other._hostname << "\e[0m" << std::endl;
// 	*this = other;
// 	return ;
// }
//
// /*	@brief Copy Assignment Operator	*/
// AddressInfo& AddressInfo::operator = (const AddressInfo& other) {
// 	std::cerr	<< "\e[3;93mAddressInfo Copy Assignment Operator called: "
// 				<< other._hostname << "\e[0m" << std::endl;
// 	if (this != &other) {
// 		_req.ai_flags = other._req.ai_flags;
// 		_req.ai_family = other._req.ai_family;
// 		_req.ai_socktype = other._req.ai_socktype;
// 		_req.ai_protocol = other._req.ai_protocol;
// 		_req.ai_addrlen = other._req.ai_addrlen;
// 		// _req.ai_addr->sa_family = other._req.ai_addr->sa_family;
// 		// for (long unsigned int i = 0; i < sizeof(_req.ai_addr->sa_data); ++i) {
// 		// 	_req.ai_addr->sa_data[i] = other._req.ai_addr->sa_data[i];
// 		// }
// 		for (int i = 0; i < LEN; ++i) {
// 			_buff[i] = other._buff[i];
// 		}
// 		for (int i = 0; i < LEN6; ++i) {
// 			_buff6[i] = other._buff6[i];
// 		}
// 	}
// 	return (*this);
// }
