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
#include "../incs/colors.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cerrno>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Server::Server(void) {
	std::cerr	<< DEBUG << "Server Constructor called" << RESET
				<< std::endl;
	// _sockfd = -1;
	// _epfd = -1;
	// _sa.sin_family = 0;
	// _sa.sin_port = 0;
	// _sa.sin_addr.s_addr = 0;
	// for (size_t z = 0; z < sizeof(_sa.sin_zero); ++z) {
	// 	_sa.sin_zero[z] = 0;
	// }
	_stop = false;
	// _kill = false;
	return;
}

/*	@brief Destructor	*/
Server::~Server(void) {
	std::cerr	<< DEBUG << "Server Destructor called" << RESET
				<< std::endl;
	if (_epfd != -1 || !_sockfd.empty() || !_clients.empty()) {
		cleanUpAllRessources();
	}
	return;
}

// /*	@brief Copy Constructor	*/
// Server::Server(const Server& other) {
// 	std::cerr	<< DEBUG << "Server Copy Constructor called" << RESET
// 				<< std::endl;
// 	*this = other;
// 	return;
// }
//
// /*	@brief Copy Assignment Operator	*/
// Server& Server::operator = (const Server& other) {
// 	std::cerr	<< DEBUG << "Server Copy Assignment Operator called" << RESET
// 				<< std::endl;
// 	if (this != &other) {}
// 	return *this;
// }

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
	epoll_event e;
	e.events = EPOLLIN;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &e);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

void Server::addWriteInterest(int fd) {
	epoll_event e;
	e.events = EPOLLIN | EPOLLOUT;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

void Server::removeWriteInterest(int fd) {
	epoll_event e;
	e.events = EPOLLIN;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		throw ModifyEPollException();
	}
	return;
}

void Server::prepareEPollInstance(void) {
	_epfd = epoll_create(1);
	if (_epfd == -1) {
		throw CreateEPollException();
	}
	std::cout	<< DEBUG << "Prepared epoll instance epfd: " << _epfd
				<< RESET << std::endl;
	return;
}

void Server::prepareListeningPort(const std::string& address, unsigned short port) {
	int result = 0;
	sockaddr_in sa;
	sa.sin_port = htons(port);
	sa.sin_family = AF_INET;
	result = inet_pton(sa.sin_family, address.c_str(), &sa.sin_addr);
	if (result == -1) {
		throw AFNotSupportedException();
	}
	if (result == 0) {
		throw InvalidAddressException();
	}
	_addr.push_back(sa);
	result = socket(_addr.back().sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (result == -1) {
		throw SocketException();
	}
	// setNonblockFlag(result);
	_sockfd.push_back(result);
	// setNonblockFlag(_sockfd.back());
	std::cout	<< DEBUG << "Created server socket fd: " << _sockfd.back()
				<< RESET << std::endl;
	result = bind(_sockfd.back(), (sockaddr*)&_addr.back(), sizeof(_addr.back()));
	if (result == -1) {
		throw BindException();
	}
	char ipstr[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(sa.sin_family, &sa.sin_addr, ipstr, INET_ADDRSTRLEN)) {
		std::cout	<< DEBUG << "Bound the socket to "
					<< ipstr << ":" << ntohs(sa.sin_port)
					<< RESET << std::endl;
	}
	result = listen(_sockfd.back(), SOMAXCONN);
	if (result == -1) {
		throw ListenException();
	}
	setReadInterest(_sockfd.back());
	std::cout	<< DEBUG << "Now listening on the socket"
				<< RESET << std::endl;
	return;
}

void Server::handleIncomingEvents(void) {
	bool run = true;
	while (run) {
		int nfds = epoll_wait(_epfd, _events, MAXEVENTS, -1);
		if (nfds == -1) {
			throw EventPollingException();
		}
		unsigned long s;
		bool found = false;
		int n;
		for (n = 0; n < nfds; ++n) {
			epoll_event e = _events[n];
			for (s = 0; s < _sockfd.size(); ++s) {
				if (e.data.fd == _sockfd[s] && e.events & EPOLLIN) {
					found = true;
					acceptConnectRequest(s);
				}
			}
			if (found) {
				break;
			}
			if (e.events & EPOLLIN) {
				handleReadEvent(e.data.fd);
			} else if (e.events & EPOLLOUT) {
				handleWriteEvent(e.data.fd);
			}
		}
		if (_clients.empty()) {
			std::cout	<< INFO << "All clients disconnected"
						<< RESET << std::endl;
			break;
			// run = false;
		}
		if (_stop == true) {
			// if (!_clients.empty()) {
			// 	std::map<int, Client*>::iterator immediate;
			// 	std::map<int, Client*>::iterator it = _clients.begin();
			// 	while (it != _clients.end()) {
			// 		immediate = it;
			// 		++it;
			// 	cleanUpClient(immediate);
			// 	}
			// }
			break;
		}
	}
	// cleanUpAllRessources();
	return;
}

// } else if (e.data.fd != _sockfd[s] && e.events & EPOLLIN) {
// 	std::cout << e.data.fd << std::endl;
// 	handleReadEvent(e.data.fd);
// } else if (e.data.fd != _sockfd[s] && e.events & EPOLLOUT) {
// 	handleWriteEvent(e.data.fd);
// }

void Server::acceptConnectRequest(int index) {
	std::cout	<< INFO << "New connection on socket fd: " << _sockfd[index]
				<< RESET << std::endl;
	Client* c = new Client();
	// std::cout << &c << "\v" << c << std::endl;
	int fd = accept(_sockfd[index], c->getAddrPointer(), c->getAddrlenPointer());
	if (fd == -1) {
		delete c;
		throw AcceptException();
	}
	_clients[fd] = c;
	setNonblockFlag(fd);
	setReadInterest(fd);
	std::cout	<< INFO << "Client #" << fd - _sockfd.back()
				<< ", endpoint " << c->getHostAddress() << ":" << c->getHostPort()
				<< RESET << std::endl;
	// pid_t pid = fork();
	// switch(pid) {
	// case -1:
	// 	std::cout << ERROR << "Error: fork: " << strerror(errno) << RESET << std::endl;
	// case 0:
	// 	close(_epfd);
	// 	close(_sockfd[index]);
	// }
	return ;
}

void Server::handleReadEvent(int fd) {
	ssize_t n = _clients[fd]->fillPendingData(fd);
	if (n < 0) {
		throw ReadDataException();
	} else if (n == 0) {
		std::cerr	<< INFO << "Connection closed by client #"
					<< fd - _sockfd.back()
					<< RESET << std::endl;
		std::map<int, Client*>::iterator it = _clients.find(fd);
		cleanUpClient(it);
		return;
	} else if (n == 3000) {
		std::cerr	<< INFO << "Connection closed by the server"
					<< RESET << std::endl;
		_stop = true;
		return;
	} else {
		_clients[fd]->queueIncomingData((size_t)n);
// DEBUG
		std::string buff = _clients[fd]->getBuffer();
		buff.erase(n);
		std::cout	<< DEBUG << "Read " << n << " bytes from client #"
					<< fd - _sockfd.back() << ":\n"
					<< RESET << buff
					<< std::endl;
		std::cout	<< DEBUG << "Current data in buffer:\n"
					<< RESET << _clients[fd]->getIncomingData()
					<< std::endl;
// DEBUG
	}
	if (!_clients[fd]->hasPendingData()) {
		std::string response = "Data Received. Ctrl+D to close the connection.\n";
		_clients[fd]->queueOutgoingData(response);
		addWriteInterest(fd);
	}
	return;
}

void Server::handleWriteEvent(int fd) {
	if (_clients[fd]->hasPendingData()) {
		int status = _clients[fd]->flushPendingData(fd);
		if (status == -1) {
			throw FlushDataException();
		}
	}
	if (!_clients[fd]->hasPendingData()) {
		removeWriteInterest(fd);
	}
	return;
}

void Server::cleanUpAllRessources(void) {
	if (!_clients.empty()) {
		std::map<int, Client*>::iterator immediate;
		std::map<int, Client*>::iterator it = _clients.begin();
		while (it != _clients.end()) {
			immediate = it;
			++it;
			cleanUpClient(immediate);
		}
	}
	if (_epfd != -1) {
		for (unsigned long s = _sockfd.size(); s > 0 ; --s) {
			if (_sockfd[s-1] != -1) {
				std::cerr	<< DEBUG << "Removing fd " << _sockfd[s-1]
							<< " (socket) from epoll instance"
							<< RESET << std::endl;
				if (epoll_ctl(_epfd, EPOLL_CTL_DEL, _sockfd[s-1], NULL) == -1) {
					std::cerr	<< WARNING << "Error during cleanup: epoll_ctl: "
								<< strerror(errno)
								<< RESET << std::endl;
				}
				std::cerr	<< DEBUG << "Closing fd " << _sockfd[s-1]
							<< " (socket)"
							<< RESET << std::endl;
				if (close(_sockfd[s-1]) == -1) {
					std::cerr	<< ERROR << "Error during cleanup: close: "
								<< strerror(errno)
								<< RESET << std::endl;
				}
			}
		}
		std::cerr	<< DEBUG << "Closing fd " << _epfd
					<< " (epoll instance epfd)"
					<< RESET << std::endl;
		if (close(_epfd) == -1) {
			std::cerr	<< WARNING << "Error during cleanup: close: "
						<< strerror(errno)
						<< RESET << std::endl;
		}
	}
	return;
}

void Server::cleanUpClient(std::map<int, Client*>::iterator it) {
	if (_epfd != -1) {
		std::cerr	<< DEBUG << "Removing fd " << it->first
					<< " (client) from epoll instance"
					<< RESET << std::endl;
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			std::cerr	<< WARNING << "Error during cleanup: epoll_ctl: "
						<< strerror(errno)
						<< RESET << std::endl;
		}
	}
	if (it->first != -1) {
		std::cerr	<< DEBUG << "Closing fd " << it->first
					<< " (client)"
					<< RESET << std::endl;
		if (close(it->first) == -1) {
			std::cerr	<< WARNING << "Error during cleanup: close: "
						<< strerror(errno)
						<< RESET << std::endl;
		}
	}
	if (it->second != NULL) {
		std::cerr	<< DEBUG << "Deleting client #"
					<< it->first - _sockfd.back()
					<< RESET << std::endl;
		delete it->second;
		it->second = NULL;
	}
	std::cerr	<< DEBUG << "Erasing container entry for above client"
				<< RESET << std::endl;
	_clients.erase(it);
}

const char* Server::AFNotSupportedException::what(void) const throw () {
	// return "AFNotSupportedException\n";
	std::cerr << ERROR << "Error: inet_pton: ";
	return strerror(errno);
}

const char* Server::InvalidAddressException::what(void) const throw () {
	std::cerr << ERROR << "Error: inet_pton: ";
	return EINADDR;
}

const char* Server::SocketException::what(void) const throw () {
	// return "SocketException\n";
	std::cerr << ERROR << "Error: socket: ";
	return strerror(errno);
}

const char* Server::BindException::what(void) const throw () {
	// return "BindException\n";
	std::cerr << ERROR << "Error: bind: ";
	return strerror(errno);
}

const char* Server::ListenException::what(void) const throw () {
	// return "ListenException\n";
	std::cerr << ERROR << "Error: listen: ";
	return strerror(errno);
}

const char* Server::CreateEPollException::what(void) const throw () {
	// return "CreateEPollException\n";
	std::cerr << ERROR << "Error: epoll_create: ";
	return strerror(errno);
}

const char* Server::ModifyEPollException::what(void) const throw () {
	// return "ModifyEPollException\n";
	std::cerr << ERROR << "Error: epoll_ctl: ";
	return strerror(errno);
}

const char* Server::EventPollingException::what(void) const throw () {
	// return "AwaitEventException\n";
	std::cerr << ERROR << "Error: epoll_wait: ";
	return strerror(errno);
}

const char* Server::GetFlagsException::what(void) const throw () {
	// return "GetFlagsException\n";
	std::cerr << ERROR << "Error: fcntl(F_GETFL): ";
	return strerror(errno);
}

const char* Server::SetFlagsException::what(void) const throw () {
	// return "SetFlagsException\n";
	std::cerr << ERROR << "Error: fcntl(F_SETFL): ";
	return strerror(errno);
}

const char* Server::AcceptException::what(void) const throw () {
	// return "AcceptException\n";
	std::cerr << ERROR << "Error: accept: ";
	return strerror(errno);
}

const char* Server::ReadDataException::what(void) const throw () {
	// return "ReadDataException\n";
	std::cerr << ERROR << "Error: recv: ";
	return strerror(errno);
}

const char* Server::FlushDataException::what(void) const throw () {
	// return "FlushDataException\n";
	std::cerr << ERROR << "Error: send: ";
	return strerror(errno);
}

// throw AcceptException();
// if (_clients.find(6) != _clients.end()) {
// 	// delete c;
// 	throw AcceptException();
// }

// _sa.sin_family = 0;
// _sa.sin_port = 0;
// _sa.sin_addr.s_addr = 0;
// memset(_sa.sin_zero, 0, sizeof(_sa.sin_zero));

// _sa.sin_addr.s_addr = htonl(SERVERADDRESS);
// _sa.sin_port = htons(PORT);

// if (address != "") {
// 	inet_pton(AF_INET, address.c_str(), &_sa.sin_addr);
// } else {
// 	_sa.sin_addr.s_addr = htonl(SERVERADDRESS);
// }
// if (port != 0) {
// 	_sa.sin_port = htons(port);
// } else {
// 	_sa.sin_port = htons(PORT);
// }

// void Server::createSocket(void) {
// 	_sockfd = socket(_sa.sin_family, SOCK_STREAM | O_NONBLOCK, 0);
// 	if (_sockfd == -1) {
// 		throw Server::SocketException();
// 	}
// 	// setNonblockFlag(_sockfd);
// 	std::cout << ERROR << "Created server socket fd: " << _sockfd << "" << RESET << std::endl;
// }

// void Server::bindSocket(void) {
// 	int status = bind(_sockfd, (sockaddr*)&_sa, sizeof(_sa));
// 	if (status == -1) {
// 		throw Server::BindException();
// 	}
// 	std::cout << ERROR << "Bound socket to localhost port " << PORT << "" << RESET << std::endl;
// }

// void Server::listenToSocket(void) {
// 	int status = listen(_sockfd, SOMAXCONN);
// 	if (status == -1) {
// 		throw Server::ListenException();
// 	}
// 	std::cout << ERROR << "Listening on port " << PORT << "" << RESET << std::endl;
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
// 	std::cout	<< ERROR << "New connection! Socket fd: "
// 				<< _sockfd << ", client fd: " << fd
// 				<< "" << RESET << std::endl;
// 	// close(fd);
// 	// _clients.erase(fd);
// 	return ;
// }

// void Server::handleReadEvent(epoll_event e) {
// 	char buffer[1024] = {0};
// 	ssize_t n = recv(e.data.fd, buffer, sizeof(buffer), 0);
// 	if (n <= 0) {
// 		throw ReadDataException();
// 		// std::cout << ERROR << "Error: reading data failed." << RESET << std::endl;
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
// 			throw FlushDataException();
//
// 			// std::cout << ERROR << "Error: flushing data failed." << RESET << std::endl;
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

// if (_clients.count(i)) {
// 	close(i);
// 	_clients.erase(i);
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

// if (!_clients.empty()) {
// 	int lowest = _clients.begin()->first;
// 	int highest = _clients.rbegin()->first;
// 	for (int i = lowest; i <= highest; ++i) {
// 		std::map<int, Client*>::iterator it = _clients.find(i);
// 		if (it == _clients.end()) {
// 			break;
// 		} else {
// 			epoll_ctl(_epfd, EPOLL_CTL_DEL, i, NULL);
// 			close(i);
// 			delete it->second;
// 			_clients.erase(it);
// 		}
// 	}
// }

// if (!_clients.empty()) {
// 	int lowest = _clients.begin()->first;
// 	int highest = _clients.rbegin()->first;
// 	for (int i = lowest; i <= highest; ++i) {
// 		std::map<int, Client*>::iterator it = _clients.find(i);
// 		if (it == _clients.end()) {
// 			break;
// 		} else {
// 			// disconnect client
// 			std::cout	<< ERROR << "Removing fd " << i
// 						<< " from epoll instance" << "" << RESET
// 						<< std::endl;
// 			if (epoll_ctl(_epfd, EPOLL_CTL_DEL, i, NULL) == -1) {
// 				std::cerr	<< ERROR << "Error during cleanup: epoll_ctl: "
// 							<< strerror(errno) << "" << RESET
// 							<< std::endl;
// 			}
// 			std::cout	<< ERROR << "And closing fd " << i
// 						<< "" << RESET << std::endl;
// 			if (close(i) == -1) {
// 				std::cerr	<< ERROR << "Error during cleanup: close: "
// 							<< strerror(errno) << "" << RESET
// 							<< std::endl;
// 			}
// 			std::cout	<< ERROR << "Deleting client #"
// 						<< i - _epfd << " (fd " << i << ")" << "" << RESET
// 						<< std::endl;
// 			delete it->second;
// 			std::cout	<< ERROR << "Erasing container entry for client #"
// 						<< i - _epfd << "" << RESET
// 						<< std::endl;
// 			_clients.erase(it);
// 			// disconnect client
// 		}
// 	}
// }

// if (!_clients.empty()) {
// 	std::map<int, Client*>::iterator immediate;
// 	std::map<int, Client*>::iterator it = _clients.begin();
// 	while (it != _clients.end()) {
// 		immediate = it;
// 		++it;
// 		epoll_ctl(_epfd, EPOLL_CTL_DEL, immediate->first, NULL);
// 		close(immediate->first);
// 		delete immediate->second;
// 		_clients.erase(immediate);
// 	}
// }

// disconnect client
// std::cerr	<< ERROR << "Removing fd " << fd
// 			<< " from epoll instance" << "" << RESET
// 			<< std::endl;
// if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
// 	std::cerr	<< ERROR << "Error during cleanup: epoll_ctl: "
// 				<< strerror(errno) << "" << RESET
// 				<< std::endl;
// }
// std::cerr	<< ERROR << "And closing fd " << fd
// 			<< "" << RESET << std::endl;
// if (close(fd) == -1) {
// 	std::cerr	<< ERROR << "Error during cleanup: close: "
// 				<< strerror(errno) << "" << RESET
// 				<< std::endl;
// }
// std::cerr	<< ERROR << "Deleting client #"
// 			<< fd - _epfd << " (fd " << fd << ")" << "" << RESET
// 			<< std::endl;
// delete it->second;
// std::cerr	<< ERROR << "Erasing container entry for client #"
// 			<< fd - _epfd << "" << RESET
// 			<< std::endl;
// _clients.erase(it);

// std::cerr	<< ERROR << "Removing fd " << immediate->first
// 			<< " from epoll instance" << "" << RESET
// 			<< std::endl;
// if (epoll_ctl(_epfd, EPOLL_CTL_DEL, immediate->first, NULL) == -1) {
// 	std::cerr	<< ERROR << "Error during cleanup: epoll_ctl: "
// 				<< strerror(errno) << "" << RESET
// 				<< std::endl;
// }
// std::cerr	<< ERROR << "And closing fd " << immediate->first << "" << RESET
// 			<< std::endl;
// if (close(immediate->first) == -1) {
// 	std::cerr	<< ERROR << "Error during cleanup: close: "
// 				<< strerror(errno) << "" << RESET
// 				<< std::endl;
// }
// std::cerr	<< ERROR << "Deleting client #" << immediate->first - _epfd
// 			<< " (fd " << immediate->first << ")" << "" << RESET
// 			<< std::endl;
// delete immediate->second;
// std::cerr	<< ERROR << "Erasing container entry for client #"
// 			<< immediate->first - _epfd << "" << RESET
// 			<< std::endl;
// _clients.erase(immediate);
