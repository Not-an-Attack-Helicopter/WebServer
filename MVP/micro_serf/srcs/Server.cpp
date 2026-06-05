/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 07:17:56 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/04 07:17:57 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Server.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Server::Server(void) {
	std::cerr	<< "\e[3;93mServer Constructor called\e[0m"
				<< std::endl;
	_sockfd = 0;
	_epfd = 0;
	_sa.sin_family = 0;
	_sa.sin_port = 0;
	_sa.sin_addr.s_addr = 0;
	for (unsigned long i = 0; i < sizeof(_sa.sin_zero); ++i) {
		_sa.sin_zero[i] = 0;
	}
	return;
}

/*	@brief Destructor	*/
Server::~Server(void) {
	std::cerr	<< "\e[3;93mServer Destructor called\e[0m"
				<< std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Server::Server(const Server& other) {
	std::cerr	<< "\e[3;93mServer Copy Constructor called\e[0m"
				<< std::endl;
	*this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Server& Server::operator = (const Server& other) {
	std::cerr	<< "\e[3;93mServer Copy Assignment Operator called\e[0m"
				<< std::endl;
	if (this != &other) {}
	return *this;
}

void Server::setNonblockFlag(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		throw GetFlagsException();
	}
	int status = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (status == -1) {
		throw SetFlagsException();
	}
	return;
}

void Server::setReadInterest(int fd) {
	_ev.events = EPOLLIN;
	_ev.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_ev);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

void Server::prepareListeningPort(void) {
	int status = 0;
	_sa.sin_addr.s_addr = htonl(SERVERADDRESS);
	_sa.sin_port = htons(PORT);
	_sa.sin_family = AF_INET;
	_sockfd = socket(_sa.sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (_sockfd == -1) {
		throw SocketException();
	}
	// setNonblockFlag(_sockfd);
	std::cout	<< "\e[3;93mCreated server socket fd: " << _sockfd
				<< "\e[0m" << std::endl;
	status = bind(_sockfd, (sockaddr*)&_sa, sizeof(_sa));
	if (status == -1) {
		throw BindException();
	}
	std::cout	<< "\e[3;93mBound socket to localhost:" << PORT
				<< "\e[0m" << std::endl;
	status = listen(_sockfd, BACKLOG);
	if (status == -1) {
		throw ListenException();
	}
	std::cout	<< "\e[3;93mListening on port " << PORT
				<< "\e[0m" << std::endl;
	return;
}

void Server::prepareEPollInstance(void) {
	_epfd = epoll_create(1);
	if (_epfd == -1) {
		throw CreateEPollException();
	}
	setReadInterest(_sockfd);
	std::cout	<< "\e[3;93mPrepared epoll instance epfd: " << _epfd
				<< "\e[0m" << std::endl;
}

void Server::handleIncomingEvents(void) {
	while (true) {
		int nfds = epoll_wait(_epfd, _events, MAXEVENTS, -1);
		if (nfds == -1) {
			throw EventPollingException();
		}
		for (int i = 0; i < nfds; ++i) {
			if (_events[i].data.fd == _sockfd) {
				acceptConnectRequest();
				// break;
			// } else {
				// handle exisiting client
			}
		}
		break;
	}
	close(_epfd);
	close(_sockfd);
}

void Server::acceptConnectRequest(void) {
	// std::cout << "creating client..." << std::endl;
	Client client;
	// std::cout << "client created" << std::endl;
	int fd = accept(_sockfd, client.getAddrPointer(), client.getAddrlenPointer());
	if (fd == -1) {
		throw AcceptException();
	}
	setNonblockFlag(fd);
	// std::cout << "client will be inserted into container..." << std::endl;
	_clients[fd] = client;
	// _clients.insert({fd, client});
	// std::cout << "client successfully inserted into container" << std::endl;
	setReadInterest(fd);
	std::cout	<< "\e[3;93mNew connection! Socket fd: "
				<< _sockfd << ", client fd: " << fd
				<< "\e[0m" << std::endl;
	close(fd);
	_clients.erase(fd);
	return ;
}

const char* Server::SocketException::what(void) const throw () {
	// return "SocketException\n";
	return strerror(errno);
}

const char* Server::BindException::what(void) const throw () {
	// return "BindException\n";
	return strerror(errno);
}

const char* Server::ListenException::what(void) const throw () {
	// return "ListenException\n";
	return strerror(errno);
}

const char* Server::CreateEPollException::what(void) const throw () {
	// return "CreateEPollException\n";
	return strerror(errno);
}

const char* Server::ModifyEPollException::what(void) const throw () {
	// return "ModifyEPollException\n";
	return strerror(errno);
}

const char* Server::EventPollingException::what(void) const throw () {
	// return "AwaitEventException\n";
	return strerror(errno);
}

const char* Server::GetFlagsException::what(void) const throw () {
	// return "GetFlagsException\n";
	return strerror(errno);
}

const char* Server::SetFlagsException::what(void) const throw () {
	// return "SetFlagsException\n";
	return strerror(errno);
}

const char* Server::AcceptException::what(void) const throw () {
	// return "AcceptException\n";
	return strerror(errno);
}

// _sa.sin_family = 0;
// _sa.sin_port = 0;
// _sa.sin_addr.s_addr = 0;
// memset(_sa.sin_zero, 0, sizeof(_sa.sin_zero));

// const char* Server::FcntlException::what(void) const throw () {
// 	// return "SetFlagsException\n";
// 	return strerror(errno));
// }

// void Server::createSocket(void) {
// 	_sockfd = socket(_sa.sin_family, SOCK_STREAM | O_NONBLOCK, 0);
// 	if (_sockfd == -1) {
// 		throw Server::SocketException();
// 	}
// 	// setNonblockFlag(_sockfd);
// 	std::cout << "\e[3;93mCreated server socket fd: " << _sockfd << "\e[0m" << std::endl;
// }

// void Server::bindSocket(void) {
// 	int status = bind(_sockfd, (sockaddr*)&_sa, sizeof(_sa));
// 	if (status == -1) {
// 		throw Server::BindException();
// 	}
// 	std::cout << "\e[3;93mBound socket to localhost port " << PORT << "\e[0m" << std::endl;
// }

// void Server::listenToSocket(void) {
// 	int status = listen(_sockfd, BACKLOG);
// 	if (status == -1) {
// 		throw Server::ListenException();
// 	}
// 	std::cout << "\e[3;93mListening on port " << PORT << "\e[0m" << std::endl;
// }
