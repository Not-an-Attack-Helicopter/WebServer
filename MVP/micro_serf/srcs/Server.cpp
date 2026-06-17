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
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include "../incs/types.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Server& Server::instance(void) {
	static Server instance;
	return instance;
}

void Server::setNonblockFlag(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		// throw GetFileStatusException();
		throw std::runtime_error("fcntl(F_GETFL): " + std::string(strerror(errno)));
	}
	int status = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (status == -1) {
		// throw SetFileStatusException();
		throw std::runtime_error("fcntl(F_SETFL): " + std::string(strerror(errno)));
	}
	return;
}

void Server::setReadInterest(int fd) {
	epoll_event e;
	e.events = EPOLLIN;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &e);
	if (status == -1) {
		// throw ModifyEPollException();
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}
	return;
}

void Server::addWriteInterest(int fd) {
	epoll_event e;
	e.events = EPOLLIN | EPOLLOUT;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw ModifyEPollException();
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}
	return;
}

void Server::removeWriteInterest(int fd) {
	epoll_event e;
	e.events = EPOLLIN;
	e.data.fd = fd;
	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw ModifyEPollException();
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}
	return;
}

void Server::prepareEPollInstance(void) {
	_epfd = epoll_create(1);
	if (_epfd == -1) {
		// throw CreateEPollException();
		throw std::runtime_error("epoll_create: " + std::string(strerror(errno)));
	}
	log.debug("Prepared epoll instance epfd: " + i2a(_epfd));
	return;
}

void Server::prepareListeningPort(const std::string& address, unsigned short port) {
	int opt = 1;
	int result = 0;
	sockaddr_in sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sin_port = htons(port);
	sa.sin_family = AF_INET;
	result = inet_pton(sa.sin_family, address.c_str(), &sa.sin_addr);
	if (result == -1) {
		// throw AFNotSupportedException();
		throw std::runtime_error("inet_pton: " + std::string(strerror(errno)));
	}
	if (result == 0) {
		// throw InvalidAddressException();
		throw std::runtime_error("inet_pton: " + std::string(INVALID_ADDR));
	}
	_addr.push_back(sa);
	result = socket(_addr.back().sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (result == -1) {
		// throw SocketException();
		throw std::runtime_error("socket: " + std::string(strerror(errno)));
	}
	_sockfd.push_back(result);
	result = setsockopt(_sockfd.back(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == -1) {
		// throw SetSockOptionException();
		throw std::runtime_error("setsockopt: " + std::string(strerror(errno)));
	}
	log.debug("Created server socket fd: " + i2a(_sockfd.back()));
	result = bind(_sockfd.back(), (sockaddr*)&_addr.back(), sizeof(_addr.back()));
	if (result == -1) {
		throw std::runtime_error("bind: " + std::string(strerror(errno)));
	}
	char ipstr[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(sa.sin_family, &sa.sin_addr, ipstr, INET_ADDRSTRLEN)) {
		log.debug("Bound the socket to " + std::string(ipstr) + ":" + i2a(ntohs(sa.sin_port)));
	}
	result = listen(_sockfd.back(), SOMAXCONN);
	if (result == -1) {
		// throw ListenException();
		throw std::runtime_error("listen: " + std::string(strerror(errno)));
	}
	setReadInterest(_sockfd.back());
	log.debug("Now listening on the socket");
	return;
}

void Server::handleIncomingEvents(void) {
	log.info("Awaiting new connection");
	while (true) {
		int nfds = epoll_wait(_epfd, _events, MAXEVENTS, -1);
		if (nfds == -1) {
			// throw EventPollingException();
			throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));
		}
		for (int n = 0; n < nfds; ++n) {
			int fd = _events[n].data.fd;
			epoll_event ev = _events[n];
			uint32_t events = ev.events;
			bool isSocket = false;
			for (size_t s = 0; s < _sockfd.size(); ++s) {
				if (fd == _sockfd[s] && events & EPOLLIN) {
					isSocket = true;
					break;
				}
			}
			if (!isSocket) {
				if (events & EPOLLIN) {
					handleReadEvent(fd);
				}
				if (events & EPOLLOUT) {
					handleWriteEvent(fd);
				}
			} else {
				acceptConnectRequest(fd);
			}
		}
		// DEBUG
		if (_clients.empty()) {
			log.info("All clients disconnected");
			// break;
		}
		if (_stop == true) {
			break;
		}
		// DEBUG
	}
	return;
}

void Server::acceptConnectRequest(int socket_fd) {
	log.info("New connection on socket fd: " + i2a(socket_fd));
	Client* c = new Client();
	int client_fd = accept(socket_fd, c->getAddrPointer(), c->getAddrlenPointer());
	if (client_fd == -1) {
		delete c;
		// throw AcceptException();
		throw std::runtime_error("accept: " + std::string(strerror(errno)));
	}
	_clients[client_fd] = c;
	setNonblockFlag(client_fd);
	setReadInterest(client_fd);
	log.info("Client #" + i2a(client_fd - _sockfd.back()) + ", endpoint "
				+ c->getHostAddress() + ":" + i2a(c->getHostPort()));
	// DEBUG We don't care about potential DoS attack vectors here
	pid_t pid = fork();
	const char* argv[] = {"echo", "Hello from the child process!", NULL};
	switch(pid) {
	case -1:
		log.error("fork: " + std::string(strerror(errno)));
		break;
	case 0:
		// cleanUpAllRessources();							// This is bad! Because all clients will be deleted
															// and all interests will be wiped from all sockets!
		// if (!_clients.empty()) {							// This is bad! The client object will be destructed.
		// 	std::map<int, Client*>::iterator immediate;
		// 	std::map<int, Client*>::iterator it = _clients.begin();
		// 	while (it != _clients.end()) {
		// 		immediate = it;
		// 		++it;
		// 		cleanUpClient(immediate);
		// 	}
		// }
		_clients.clear();
		for (int i = 0; i < MAXEVENTS; ++i) {
			_events[i].events = 0;
			_events[i].data.fd = 0;
			_events[i].data.u32 = 0;
			_events[i].data.u64 = 0;
			_events[i].data.ptr = NULL;
		}
		_sockfd.clear();
		_addr.clear();
		// epoll_ctl(_epfd, EPOLL_CTL_DEL, socket_fd, NULL);	// This is potentially bad, because in the case
															// // of disconnecting before sending 'STOP' it will
															// // not be possible to reconnect!
		close(socket_fd);
		close(_epfd);
		if (execvp(argv[0], (char* const*)argv) == -1) {
			log.error("Oh no! " + std::string(strerror(errno)));
		}
		exit(0);
		break; // Because above "statement may fall through". - The Compiler. Who has no clue, this is bog wash.
	default:
		std::cout << "Hello from the parent process!" << std::endl;
	}
	while (waitpid(-1, NULL, WNOHANG) > 0) {}
	// DEBUG
	return ;
}

void Server::handleReadEvent(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end() || it->second == NULL) {
		// throw MissingClientException();
		throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	}
	ssize_t n = it->second->fillPendingData(fd);
	if (n < 0) {
		// throw ReadDataException();
		throw std::runtime_error("recv: " + std::string(strerror(errno)));
	} else if (n == 0) {
		log.info("Connection closed by client #" + i2a(fd - _sockfd.back()));
		cleanUpClient(it);
		return;
	} else if (n == STOP) {
		log.info("Connection closed by the server");
		_stop = true;
		return;
	} else {
		it->second->queueIncomingData((size_t)n); // We already established that the client exists
// DEBUG
		std::string buff = it->second->getBuffer();
		buff.erase(n);
		log.debug("Read " + i2a(n) + " bytes from client #" + i2a(fd - _sockfd.back()) + ":\n");
		log.notice(buff);
		log.debug("Current data in buffer:\n");
		log.notice(it->second->getIncomingData());
// DEBUG
	}
	if (!it->second->hasPendingData()) { // We already established that the client exists
		std::string response = "Data Received. Ctrl+D to close the connection.\n";
		it->second->queueOutgoingData(response);
		addWriteInterest(fd);
	}
	return;
}

void Server::handleWriteEvent(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end() || it->second == NULL) {
		// throw MissingClientException();
		throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	}
	if (it->second->hasPendingData()) {
		int status = it->second->flushPendingData(fd);
		if (status == -1) {
			// throw FlushDataException();
			throw std::runtime_error("send: " + std::string(strerror(errno)));
		}
	}
	if (!it->second->hasPendingData()) {
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
	_clients.clear();
	if (_epfd != -1) {
		for (unsigned long s = _sockfd.size(); s > 0 ; --s) {
			if (_sockfd[s-1] != -1) {
				log.debug("Removing fd " + i2a(_sockfd[s-1]) + " (socket) from epoll instance");
				if (epoll_ctl(_epfd, EPOLL_CTL_DEL, _sockfd[s-1], NULL) == -1) {
					log.warning("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
				}
				log.debug("Closing fd " + i2a(_sockfd[s-1]));
				if (close(_sockfd[s-1]) == -1) {
					log.warning("Error during cleanup: close: " + std::string(strerror(errno)));
				}
			}
		}
		log.debug("Closing fd " + i2a(_epfd) + " (epoll instance epfd)");
		if (close(_epfd) == -1) {
			log.warning("Error during cleanup: close: " + std::string(strerror(errno)));
		}
		_epfd = -1;
	}
	for (int i = 0; i < MAXEVENTS; ++i) {
	_events[i].events = 0;
	_events[i].data.fd = 0;
	_events[i].data.u32 = 0;
	_events[i].data.u64 = 0;
	_events[i].data.ptr = NULL;
	}
	_sockfd.clear();
	_addr.clear();
	return;
}

void Server::cleanUpClient(std::map<int, Client*>::iterator it) {
	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(it->first) + " (client) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			log.warning("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}
	if (it->first != -1) {
		log.debug("Closing fd " + i2a(it->first) + " (client)");
		if (close(it->first) == -1) {
			log.warning("Error during cleanup: close: " + std::string(strerror(errno)));
		}
	}
	if (it->second != NULL) {
		log.debug("Deleting client #" + i2a(it->first - _sockfd.back()));
		delete it->second;
		it->second = NULL;
	}
	log.debug("Erasing container entry for above client");
	_clients.erase(it);
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Server::Server(void) {
	log.debug("Server Constructor called");
	_epfd = -1;
	_stop = false;
	return;
}

/*	@brief Destructor	*/
Server::~Server(void) {
	log.debug("Server Destructor called");
	if (_epfd != -1 || !_sockfd.empty() || !_clients.empty()) {
		cleanUpAllRessources();
	}
	return;
}

/*	@brief Copy Constructor	*/
Server::Server(const Server& other) {
	log.debug("Server Copy Constructor called");
	*this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Server& Server::operator = (const Server& other) {
	log.debug("Server Copy Assignment Operator called");
	if (this != &other) {}
	return *this;
}

// const char* Server::AFNotSupportedException::what(void) const throw () {
// 	// return "AFNotSupportedException\n";
// 	std::cerr << ERROR << "Error: inet_pton: ";
// 	return strerror(errno);
// }
//
// const char* Server::InvalidAddressException::what(void) const throw () {
// 	std::cerr << ERROR << "Error: inet_pton: ";
// 	return INVALID_ADDR;
// }
//
// const char* Server::SocketException::what(void) const throw () {
// 	// return "SocketException\n";
// 	std::cerr << ERROR << "Error: socket: ";
// 	return strerror(errno);
// }
//
// const char* Server::SetSockOptionException::what(void) const throw () {
// 	// return "SetSockOptionException\n";
// 	std::cerr << ERROR << "Error: socket: ";
// 	return strerror(errno);
// }
//
// const char* Server::BindException::what(void) const throw () {
// 	// return "BindException\n";
// 	std::cerr << ERROR << "Error: bind: ";
// 	return strerror(errno);
// }
//
// const char* Server::ListenException::what(void) const throw () {
// 	// return "ListenException\n";
// 	std::cerr << ERROR << "Error: listen: ";
// 	return strerror(errno);
// }
//
// const char* Server::CreateEPollException::what(void) const throw () {
// 	// return "CreateEPollException\n";
// 	std::cerr << ERROR << "Error: epoll_create: ";
// 	return strerror(errno);
// }
//
// const char* Server::ModifyEPollException::what(void) const throw () {
// 	// return "ModifyEPollException\n";
// 	std::cerr << ERROR << "Error: epoll_ctl: ";
// 	return strerror(errno);
// }
//
// const char* Server::EventPollingException::what(void) const throw () {
// 	// return "EventPollingException\n";
// 	std::cerr << ERROR << "Error: epoll_wait: ";
// 	return strerror(errno);
// }
//
// const char* Server::GetFileStatusException::what(void) const throw () {
// 	// return "GetFileStatusException\n";
// 	std::cerr << ERROR << "Error: fcntl(F_GETFL): ";
// 	return strerror(errno);
// }
//
// const char* Server::SetFileStatusException::what(void) const throw () {
// 	// return "SetFileStatusException\n";
// 	std::cerr << ERROR << "Error: fcntl(F_SETFL): ";
// 	return strerror(errno);
// }
//
// const char* Server::AcceptException::what(void) const throw () {
// 	// return "AcceptException\n";
// 	std::cerr << ERROR << "Error: accept: ";
// 	return strerror(errno);
// }
//
// const char* Server::MissingClientException::what(void) const throw () {
// 	// return "MissingClientException\n";
// 	std::cerr << ERROR << "Error: client lookup: ";
// 	return NFIND_CLIENT;
// }
//
// const char* Server::ReadDataException::what(void) const throw () {
// 	// return "ReadDataException\n";
// 	std::cerr << ERROR << "Error: recv: ";
// 	return strerror(errno);
// }
//
// const char* Server::FlushDataException::what(void) const throw () {
// 	// return "FlushDataException\n";
// 	std::cerr << ERROR << "Error: send: ";
// 	return strerror(errno);
// }
