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
#include <arpa/inet.h>
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

void Server::addWriteInterest(int fd) {
	_ev.events = EPOLLIN | EPOLLOUT;
	_ev.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

void Server::removeWriteInterest(int fd) {
	_ev.events = EPOLLIN;
	_ev.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

// void Server::prepareListeningPort(void) {
void Server::prepareListeningPort(const std::string& address) {
	int status = 0;
	// _sa.sin_addr.s_addr = htonl(SERVERADDRESS);
	if (address == "") {
		_sa.sin_addr.s_addr = htonl(SERVERADDRESS);
	} else {
		inet_pton(AF_INET, address.c_str(), &_sa.sin_addr);
	}
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
	char buff[16] = {0};
	inet_ntop(_sa.sin_family, &_sa.sin_addr, buff, INET_ADDRSTRLEN);
	std::cout	<< "\e[3;93mBound the socket to " << buff << ":" << PORT
				<< "\e[0m" << std::endl;
	status = listen(_sockfd, BACKLOG);
	if (status == -1) {
		throw ListenException();
	}
	std::cout	<< "\e[3;93mNow listening on server port: " << PORT
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
			epoll_event e = _events[i];
			if (e.data.fd == _sockfd && e.events & EPOLLIN) {
				acceptConnectRequest();
			} else if (e.data.fd != _sockfd && e.events & EPOLLIN) {
				handleReadEvent(e);
			} else if (e.data.fd != _sockfd && e.events & EPOLLOUT) {
				handleWriteEvent(e);
			}
		}
	}
	cleanUpAllRessources();
}

void Server::acceptConnectRequest(void) {
	Client c;
	int fd = accept(_sockfd, c.getAddrPointer(), c.getAddrlenPointer());
	if (fd == -1) {
		throw AcceptException();
	}
	setNonblockFlag(fd);
	_clients[fd] = c;
	setReadInterest(fd);
	std::cout	<< "\e[3;93mNew connection! Socket fd: "
				<< _sockfd << ", client fd: " << fd
				<< "\e[0m" << std::endl;
	return ;
}

void Server::handleReadEvent(epoll_event e) {
	char buffer[1024] = {0};
	ssize_t n = recv(e.data.fd, buffer, sizeof(buffer), 0);
	if (n <= 0) {
		cleanUpAllRessources();
		throw ReadDataException();
	} else {
		_clients[e.data.fd].queueIncomingData(buffer);
	}
	if (!_clients[e.data.fd].hasPendingData()) {
		std::string response = "Data Received. Ctrl+D to close.\n";
		_clients[e.data.fd].queueOutgoingData(response);
		addWriteInterest(e.data.fd);
	}
}

void Server::handleWriteEvent(epoll_event e) {
	if (_clients[e.data.fd].hasPendingData()) {
		int status = _clients[e.data.fd].flushPendingData(e.data.fd);
		if (status == -1) {
			cleanUpAllRessources();
			throw FlushDataException();
		}
	}
	if (!_clients[e.data.fd].hasPendingData()) {
		removeWriteInterest(e.data.fd);
	}
}

void Server::cleanUpAllRessources(void) {
	if (!_clients.empty()) {
		int lowest = _clients.begin()->first;
		int highest = _clients.rbegin()->first;
		for (int i = lowest; i <= highest; ++i) {
			std::map<int, Client>::iterator it = _clients.find(i);
			if (it != _clients.end()) {
				epoll_ctl(_epfd, EPOLL_CTL_DEL, i, NULL);
				close(i);
				_clients.erase(it);
			}
		}
	}
	epoll_ctl(_epfd, EPOLL_CTL_DEL, _sockfd, NULL);
	close(_sockfd);
	close(_epfd);
	return;
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

const char* Server::ReadDataException::what(void) const throw () {
	// return "ReadDataException\n";
	return strerror(errno);
}

const char* Server::FlushDataException::what(void) const throw () {
	// return "FlushDataException\n";
	return strerror(errno);
}

// _sa.sin_family = 0;
// _sa.sin_port = 0;
// _sa.sin_addr.s_addr = 0;
// memset(_sa.sin_zero, 0, sizeof(_sa.sin_zero));

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

// void Server::acceptConnectRequest(void) {
// 	// std::cout << "creating client..." << std::endl;
// 	Client client;
// 	// std::cout << "client created" << std::endl;
// 	int fd = accept(_sockfd, client.getAddrPointer(), client.getAddrlenPointer());
// 	if (fd == -1) {
// 		throw AcceptException();
// 	}
// 	// std::cout << "provided by accept(): " << fd << std::endl;
// 	setNonblockFlag(fd);
// 	// std::cout << "client will be inserted into container..." << std::endl;
// 	_clients[fd] = client;
// 	// _clients.insert({fd, client});
// 	// std::cout << "client successfully inserted into container" << std::endl;
// 	setReadInterest(fd);
// 	std::cout	<< "\e[3;93mNew connection! Socket fd: "
// 				<< _sockfd << ", client fd: " << fd
// 				<< "\e[0m" << std::endl;
// 	// close(fd);
// 	// _clients.erase(fd);
// 	return ;
// }

// void Server::handleReadEvent(epoll_event e) {
// 	char buffer[1024] = {0};
// 	ssize_t n = recv(e.data.fd, buffer, sizeof(buffer), 0);
// 	if (n <= 0) {
// 		cleanUpAllRessources();
// 		throw ReadDataException();
// 		// std::cout << "\e[31mError: reading data failed.\e[0m" << std::endl;
// 		// epoll_ctl(_epfd, EPOLL_CTL_DEL, e.data.fd, NULL);
// 		// close(e.data.fd);
// 		// _clients.erase(e.data.fd);
// 		// running = false;
// 	} else {
// 		_clients[e.data.fd].queueIncomingData(buffer);
// 		// std::cout	<< "Read " << n << " bytes from client #" << e.data.fd << ":\n"
// 		// 			<< buffer << std::endl;
// 		// std::cout	<< "Current data in buffer :\n"
// 		// 			<< _clients[e.data.fd].getIncomingData() << std::endl;
// 	}
// 	if (!_clients[e.data.fd].hasPendingData()) {
// 		std::string response = "Data Received.\n";
// 		_clients[e.data.fd].queueOutgoingData(response);
// 		addWriteInterest(e.data.fd);
// 	}
// }

// void Server::handleWriteEvent(epoll_event e) {
// 	if (_clients[e.data.fd].hasPendingData()) {
// 		// ssize_t n = send(e.data.fd, &_clients[e.data.fd].getOutgoingData(), sizeof(_clients[e.data.fd].getOutgoingData()), 0);
// 		// if (n <= -1) {
// 		int status = _clients[e.data.fd].flushPendingData(e.data.fd);
// 		if (status == -1) {
// 			cleanUpAllRessources();
// 			throw FlushDataException();
//
// 			// std::cout << "\e[31mError: flushing data failed.\e[0m" << std::endl;
// 			// epoll_ctl(_epfd, EPOLL_CTL_DEL, e.data.fd, NULL);
// 			// close(e.data.fd);
// 			// _clients.erase(e.data.fd);
// 			// running = false;
//
// 		// } else {
// 		// 	std::cout << (_clients[e.data.fd].getOutgoingData().empty() ? "Buffer empty" : "Buffer not empty") << std::endl;
// 		// 	std::cout << "-----\n" << _clients[e.data.fd].getOutgoingData() << "-----\n" << std::endl;
// 		// }
// 		// } else if (!_clients[e.data.fd].hasPendingData()) {
// 		// 	removeWriteInterest(e.data.fd);
// 		}
// 		// _clients[e.data.fd].getOutgoingData().erase(0 , n);
// 	}
// 	if (!_clients[e.data.fd].hasPendingData()) {
// 		removeWriteInterest(e.data.fd);
// 	}
// }

// void Server::cleanUpAllRessources(void) {
// 	if (!_clients.empty()) {
// 		int lowest = _clients.begin()->first;
// 		int highest = _clients.rbegin()->first;
// 		for (int i = lowest; i <= highest; ++i) {
// 			std::map<int, Client>::iterator it = _clients.find(i);
// 			if (it != _clients.end()) {
// 				epoll_ctl(_epfd, EPOLL_CTL_DEL, i, NULL);
// 				close(i);
// 				_clients.erase(it);
// 			}
// 		}
// 	}
// 	epoll_ctl(_epfd, EPOLL_CTL_DEL, _sockfd, NULL);
// 	close(_sockfd);
// 	close(_epfd);
// 	return;
// }

// if (_clients.count(i)) {
// 	close(i);
// 	_clients.erase(i);
// }

// const char* Server::FcntlException::what(void) const throw () {
// 	// return "SetFlagsException\n";
// 	return strerror(errno));
// }

// if (e.data.fd == _sockfd) {
// 	if (e.events & EPOLLIN) {
// 		acceptConnectRequest();
// 	}
// } else {
// 	// handle exisiting client
// 	if (e.events & EPOLLIN) {
// 		// handle read evenet
// 	} else if (e.events & EPOLLOUT) {
// 		// handle write event
// 	}
// }

// if (e.data.fd == _sockfd && e.events & EPOLLIN) {
// 	acceptConnectRequest();
// } else if (e.data.fd != _sockfd) {
// 	if (e.events & EPOLLIN) {
// 		// handle read evenet
// 	} else if (e.events & EPOLLOUT) {
// 		// handle write event
// 	}
// }

// switch (e.data.fd - _sockfd) {
// case 0:
// 	if (e.events & EPOLLIN) {
// 		acceptConnectRequest();
// 	}
// default:
// 	// handle exisiting client
// 	switch (e.events) {
// 	case EPOLLIN:
// 		; // handle read evenet
// 	case EPOLLIN | EPOLLOUT:
// 		; // handle write event
// 	}
// }

// switch (e.events) {
// case EPOLLIN:
// 	if (e.data.fd == _sockfd) {
// 		acceptConnectRequest();
// 	} else {
// 		// handle client read event
// 	}
// case EPOLLIN | EPOLLOUT:
// 	if (e.data.fd != _sockfd) {
// 		// handle client write event
// 	}
// }

// if (e.data.fd == _sockfd) {
// 	if (e.events & EPOLLIN) {
// 		acceptConnectRequest();
// 	}
// } else {
// 	// handle exisiting client
// 	switch (e.events) {
// 	case EPOLLIN:
// 		; // handle read evenet
// 	case EPOLLIN | EPOLLOUT:
// 		; // handle write event
// 	}
// }
