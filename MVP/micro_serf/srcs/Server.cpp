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
// #include <cstring>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Server::Server(void) {
	std::cerr << "\e[3;93mServer Constructor called\e[0m" << std::endl;
	_status = 0;
	_sa.sin_family = AF_INET;
	_sa.sin_port = htons(PORT);
	_sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	for (int i = 0; i < sizeof(_sa.sin_zero); ++i) {
		_sa.sin_zero[i] = 0;
	}
	_sockfd = socket(_sa.sin_family, SOCK_STREAM, 0);
	if (_sockfd == -1) {
		throw (Server::SocketException());
	}
	setNonblockFlag(_sockfd);
	std::cout << "\e[3;93mCreated server socket fd: " << _sockfd << "\e[0m" << std::endl;
}


/*	@brief Destructor	*/
Server::~Server(void) {
	std::cerr << "\e[3;93mServer Destructor called\e[0m" << std::endl;
	return ;
}

/*	@brief Copy Constructor	*/
Server::Server(const Server& other) {
	std::cerr	<< "\e[3;93mServer Copy Constructor called\e[0m" << std::endl;
	// *this = other;
	return ;
}

/*	@brief Copy Assignment Operator	*/
Server& Server::operator = (const Server& other) {
	std::cerr	<< "\e[3;93mServer Copy Assignment Operator called\e[0m" << std::endl;
	if (this != &other) {}
	return (*this);
}

void Server::setNonblockFlag(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		throw (Server::GetFlagsException());
	}
	_status = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (_status == -1) {
		throw (SetFlagsException());
	}
}

// void Server::bindSocket(void) {
// 	_status = bind(_sockfd, (struct sockaddr*)&_sa, sizeof(_sa));
// 	if (_status == -1) {
// 		throw (Server::BindException());
// 	}
// }

// void Server::listenToSocket(void) {
// 	_status = listen(_sockfd, BACKLOG);
// 	if (_status == -1) {
// 		throw (Server::ListenException());
// 	}
// }

void Server::prepareListeningPort(void) {
	_status = bind(_sockfd, (struct sockaddr*)&_sa, sizeof(_sa));
	if (_status == -1) {
		throw (Server::BindException());
	}
	std::cout << "\e[3;93mBound socket to localhost port " << PORT << "\e[0m" << std::endl;
	_status = listen(_sockfd, BACKLOG);
	if (_status == -1) {
		throw (Server::ListenException());
	}
	std::cout << "\e[3;93mListening on port " << PORT << "\e[0m" << std::endl;
}

void Server::prepareEPollInstance(void) {
	_ev.events = EPOLLIN;
	_ev.data.fd = _sockfd;
	_epfd = epoll_create(1);
	if (_epfd == -1) {
		throw (Server::EPollCreateException());
	}
	_status = epoll_ctl(_epfd, EPOLL_CTL_ADD, _sockfd, &_ev);
	if (_status == -1) {
		throw (Server::EPollControlException());
	}
	std::cout << "\e[3;93mPrepared epoll instance epfd: " << _epfd << "\e[0m" << std::endl;
}

void Server::acceptConnectionRequest(int fd) {
	// for (int i = 0; i < nfds; ++i) {
	// 	int			fd = _events[i].data.fd;
	// 	uint32_t	events = _events[i].events;
	// 	if (_events[i].data.fd == _sockfd && (_events[i].events & EPOLLIN)) {
			Client _clients[fd];
			// _clients[fd].sockfd = accept(_sockfd, (struct sockaddr*)&_clients[fd].sa, &_clients[fd].addrLen);
			// if (_clients[fd].sockfd == -1) {
			int tmp = accept(_sockfd, (struct sockaddr*)&_clients[fd].sa, &_clients[fd].addrLen);
			if (tmp == -1) {
				throw (Server::AcceptException());
			}
			// setNonblockFlag(_clients[fd].sockfd);
			setNonblockFlag(tmp);
			std::cout	<< "\e[3;93mNew connection! Socket fd: " << _sockfd
						<< ", client fd: " << tmp
						<< "\e[0m" << std::endl;
			_ev.events = EPOLLIN;
			// _ev.data.fd = _clients[fd].sockfd;
			// epoll_ctl(_epfd, EPOLL_CTL_ADD, _clients[fd].sockfd, &_ev);
			_ev.data.fd = tmp;
			epoll_ctl(_epfd, EPOLL_CTL_ADD, tmp, &_ev);
	// 	} else {
	// 		Client _clients[i];
	// 		_clients[i].queueResponse("data\n");
	// 	}
	// }
	return ;
}

const char* Server::SocketException::what(void) const throw () {
	// return ("SocketException\n");
	return (strerror(errno));
}

const char* Server::GetFlagsException::what(void) const throw () {
	// return ("GetFlagsException\n");
	return (strerror(errno));
}

const char* Server::SetFlagsException::what(void) const throw () {
	// return ("SetFlagsException\n");
	return (strerror(errno));
}

const char* Server::EPollCreateException::what(void) const throw () {
	// return ("SetFlagsException\n");
	return (strerror(errno));
}

const char* Server::EPollControlException::what(void) const throw () {
	// return ("SetFlagsException\n");
	return (strerror(errno));
}

const char* Server::BindException::what(void) const throw () {
	// return ("BindException\n");
	return (strerror(errno));
}

const char* Server::ListenException::what(void) const throw () {
	// return ("ListenException\n");
	return (strerror(errno));
}

const char* Server::AcceptException::what(void) const throw () {
	// return ("AcceptException\n");
	return (strerror(errno));
}

	// _sa.sin_family = 0;
	// _sa.sin_port = 0;
	// _sa.sin_addr.s_addr = 0;
	// memset(_sa.sin_zero, 0, sizeof(_sa.sin_zero));
