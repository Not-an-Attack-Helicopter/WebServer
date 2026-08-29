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
#include "../incs/Dispatcher.hpp"
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
// #include <sys/socket.h>
// #include <sys/epoll.h>
#include <arpa/inet.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <stdexcept>
#include <unistd.h>
// #include <iostream>
#include <fcntl.h>
// #include <cstring>
// #include <cstdlib>
// #include <cerrno>

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
		throw std::runtime_error("fcntl(F_GETFL): " + std::string(strerror(errno)));
	}

	int status = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (status == -1) {
		throw std::runtime_error("fcntl(F_SETFL): " + std::string(strerror(errno)));
	}

	return;
}

void Server::setRDWRInterest(int fd) {

	epoll_event e;
	e.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
	e.data.fd = fd;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	return;
}

void Server::dropWriteInterest(int fd) {

	epoll_event e;
	e.events = EPOLLRDHUP;
	e.data.fd = fd;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	return;
}

void Server::setPollInterest(FD fd) {

	epoll_event e;
	e.data.fd = fd.fd;
	e.events = 0;
	if (fd.type == FD::SOCKET) {
		e.events = EPOLLIN | EPOLLRDHUP;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd.fd, &e);
	if (status == -1) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	return;
}

void Server::setRDONLYInterest(FD fd) {

	epoll_event e;
	e.data.fd = fd.fd;
	if (fd.type == FD::PIPE) {
		e.events = EPOLLIN;
	} else if (fd.type == FD::SOCKET) {
		e.events = EPOLLIN | EPOLLRDHUP;
	} else {
		e.events = 0;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd.fd, &e);
	if (status == -1) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	return;
}

void Server::setWRONLYInterest(FD fd) {

	epoll_event e;
	e.data.fd = fd.fd;
	if (fd.type == FD::PIPE) {
		e.events = EPOLLOUT;
	} else if (fd.type == FD::SOCKET) {
		e.events = EPOLLOUT | EPOLLRDHUP;
	} else {
		e.events = 0;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd.fd, &e);
	if (status == -1) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	return;
}

void Server::prepareEPollInstance(void) {

	_epfd = epoll_create(1);
	if (_epfd == -1) {
		throw std::runtime_error("epoll_create: " + std::string(strerror(errno)));
	}

	log.debug("Prepared epoll instance epfd fd_" + i2a(_epfd));

	return;
}

void Server::prepareListeningPort(const Config::Socket& soc) {

	int opt = 1;
	int result = 0;
	sockaddr_in sa;

	std::memset(&sa, 0, sizeof(sa));
	sa.sin_port = htons(soc.port);
	sa.sin_family = AF_INET;

	result = inet_pton(sa.sin_family, soc.address.c_str(), &sa.sin_addr);
	if (result == -1) {
		throw std::runtime_error("inet_pton: " + std::string(strerror(errno)));
	}
	if (result == 0) {
		throw std::runtime_error("inet_pton: " + std::string(INVALID_ADDR));
	}

	_addr.push_back(sa);

	result = socket(_addr.back().sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (result == -1) {
		throw std::runtime_error("socket: " + std::string(strerror(errno)));
	}

	_sockets[result] = &soc;

	result = setsockopt(_sockets.rbegin()->first, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == -1) {
		throw std::runtime_error("setsockopt: " + std::string(strerror(errno)));
	}

	log.debug("Created server socket fd_" + i2a(_sockets.rbegin()->first) + "(listen_fd)");

	result = bind(_sockets.rbegin()->first, (sockaddr*)&_addr.back(), sizeof(_addr.back()));
	if (result == -1) {
		throw std::runtime_error("bind: " + std::string(strerror(errno)));
	}

	char ipstr[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(sa.sin_family, &sa.sin_addr, ipstr, INET_ADDRSTRLEN)) {
		log.debug("Bound the socket to " + std::string(ipstr) + ":" + i2a(ntohs(sa.sin_port)));
	}

	result = listen(_sockets.rbegin()->first, SOMAXCONN);
	if (result == -1) {
		throw std::runtime_error("listen: " + std::string(strerror(errno)));
	}

	FD fildes = {_sockets.rbegin()->first, FD::SOCKET};
	setPollInterest(fildes);

	log.debug("Now listening on listen_fd fd_" + i2a(_sockets.rbegin()->first));

	return;

}

void Server::handleEvents(void) {

	log.info("Awaiting new connection");

	for (;;) {

		int nfds = epoll_wait(_epfd, _events, MAX_EPOLL_EVENTS, EPOLL_WAIT_TIMEOUT_MS);

		switch (nfds) {

		case -1:
			// throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));
			log.error("epoll_wait: " + std::string(strerror(errno)));
// DEBUG BEGIN
		case 0:
			log.debug("Timeout: no events");
			break;

		default:
			dumpEvents(nfds, _events);
			warnHighEventLoad(nfds, MAX_EPOLL_EVENTS);
// DEBUG END
		}

		for (int n = 0; n < nfds; ++n) {
			int fd = _events[n].data.fd;
			epoll_event ev = _events[n];
			uint32_t events = ev.events;
			bool is_listen_socket = false;
			bool is_socket_closed = false;
			std::map<int, const Config::Socket*>::iterator it = _sockets.begin();

			while (it != _sockets.end()) {
				if (fd == it->first && events & EPOLLIN) {
					is_listen_socket = true;
					break;
				}
				++it;
			}

			if (!is_listen_socket) {

				if (events & EPOLLERR)
					handleSocketError(fd);
				else if (events & EPOLLHUP)
					handleHangup(fd);
				else if (events & EPOLLRDHUP)
					handleRemoteHangup(fd);
				else if (events & EPOLLIN)
					is_socket_closed = handleReadEvent(fd);

				if (!is_socket_closed && (events & EPOLLOUT))
					handleWriteEvent(fd);

			} else {
				acceptConnectRequest(it->first, it->second);
			}

		}
// DEBUG BEGIN
		if (_stop == true) {
			break;
		}
// DEBUG END
		std::map<int, Client*>::iterator immediate;
		std::map<int, Client*>::iterator it = _clients.begin();

		while (it != _clients.end()) {
			immediate = it;
			++it;

			if (immediate->second->isTimedOut()) {
// DEBUG BEGIN
				log.debug("Client fd_" + i2a(immediate->first)
				+ " idle time: " + i2a(immediate->second->getIdleTime()) + "s");
// DEBUG END
				log.warn("Client fd_" + i2a(immediate->first) + " timed out");

				if (immediate->second->getState() == Client::RECEIVING_HEADERS) {
					immediate->second->setState(Client::PENDING_RESPONSE);
					immediate->second->pushResponse();
					dispatch.errorPage(immediate->second->getCurrentRequest().resolved.location,
									   immediate->second->getCurrentResponse(),
									   immediate->second->getCurrentRequest().headers_only,
									   REQUEST_TIMEOUT);
					immediate->second->popRequest();
					FD fildes = {immediate->first, FD::SOCKET};
					setWRONLYInterest(fildes);
					immediate->second->markForTermination();
				} else {
					cleanUpClient(immediate);
				}
// DEBUG BEGIN
				if (_clients.empty()) {
					log.info("All clients disconnected");
				}
// DEBUG END
			}

		}

	}

	return;

}

void Server::acceptConnectRequest(int listen_fd, const Config::Socket* socket) {

	log.info("New connection on socket fd_" + i2a(listen_fd));

	Client* c = new Client(socket);

	int client_fd = accept(listen_fd, &c->getAddr(), &c->getAddrlen());
	if (client_fd == -1) {

		int err_no = errno;

		delete c;

		if (err_no == EAGAIN || err_no == EWOULDBLOCK) {
			return;
		} else {
			// throw std::runtime_error("accept: " + std::string(strerror(errno)));
			log.error("accept: " + std::string(strerror(errno)));
		}

	}

	_clients[client_fd] = c;

	setNonblockFlag(client_fd);
	FD fildes = {client_fd, FD::SOCKET};
	setPollInterest(fildes);

	log.info("Client fd_" + i2a(client_fd) + ", endpoint "
				+ c->getHostAddress() + ":" + i2a(c->getHostPort()));
	return;

}

void Server::handleSocketError(int fd) {

	int error = 0;
	socklen_t len = sizeof(error);

	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
		log.error("getsockopt(SO_ERROR): " + std::string(strerror(errno)));
	} else if (error != 0) {
		log.error("socket error: " + std::string(strerror(error)));
	}

	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it == _clients.end() || it->second == NULL) {
		// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
		log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	}

	cleanUpClient(it);
	return;

}

void Server::handleHangup(int fd) {

	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it == _clients.end() || it->second == NULL) {
		// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
		log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	}

	cleanUpClient(it);
	return;

}

void Server::handleRemoteHangup(int fd) {

	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it == _clients.end() || it->second == NULL) {
		// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
		log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	}

	cleanUpClient(it);
	return;

}

bool Server::handleReadEvent(int fd) {

	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it == _clients.end() || it->second == NULL) {
		// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
		log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	}

	Client& client = *it->second;
	ssize_t bytes_received = client.queueIncomingData(fd);

	switch (bytes_received) {

// DEBUG BEGIN
	case Client::STOP:
		log.info("Connection closed by the server");
		_stop = true;
		return true;
// DEBUG END
	case -1:
		log.warn("recv: " + std::string(strerror(errno)));
		return true;
	case 0:
		log.info("Connection closed by client fd_" + i2a(fd));
		cleanUpClient(it);
// DEBUG BEGIN
		if (_clients.empty()) {
			log.info("All clients disconnected");
		}
// DEBUG END
		return true;
	default:
// TEST >>

		client.parseIncomingData();
		if (client.getState() == Client::DISPATCHING) {
			dispatch.currentRequest(client);
		}

		if (client.getState() == Client::RECEIVING_BODY) {
			client.parseIncomingData();
		}

		if (client.getState() == Client::DISPATCHING) {
			dispatch.currentRequest(client);
		}

		if (client.getState() == Client::PENDING_RESPONSE) {

			// Delete processed request from deque container
			client.popRequest();
			client.pushRequest();

			if (client.blockedFromReceiving()) {
				FD fildes = {fd, FD::SOCKET};
				setWRONLYInterest(fildes);
			} else {
				setRDWRInterest(fd);
			}

		}

// << TEST
		return false;

	}

}

void Server::handleWriteEvent(int fd) {

	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it == _clients.end() || it->second == NULL) {
		// throw std::runtime_error("client lookup: " + std::string(NFIND_CLIENT));
		log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	}

	Client& client = *it->second;
	if (client.getState() == Client::CONCLUDED ||
		client.getState() == Client::REJECTED) {
		return;
	}

// TEST BEGIN
	if (client.getState() == Client::PENDING_RESPONSE) {
		client.queueOutgoingData();
		client.popResponse();
		client.pushResponse();
	}
// TEST END
	if (client.hasPendingData()) {

		client.flushPendingData(fd);
	}

	FD fildes = {fd, FD::SOCKET};

	switch (client.getState()) {

	case Client::IDLE:
		setRDONLYInterest(fildes);
		break;
	case Client::ERROR:
		cleanUpClient(it);
		break;
	case Client::REJECTED:
		dropWriteInterest(fd);
		break;
	case Client::CONCLUDED:
		cleanUpClient(it);
	default:
		break;

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

	if (!_sockets.empty()) {

		std::map<int, const Config::Socket*>::iterator immediate;
		std::map<int, const Config::Socket*>::iterator it = _sockets.begin();

		while (it != _sockets.end()) {
			immediate = it;
			++it;
			cleanUpSocket(immediate);
		}

	}
	_sockets.clear();

	if (_epfd != -1) {

		log.debug("Closing fd " + i2a(_epfd) + " (epoll instance epfd)");
		if (close(_epfd) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
		_epfd = -1;
	}

	for (int i = 0; i < MAX_EPOLL_EVENTS; ++i) {
		_events[i].events = 0;
		_events[i].data.fd = 0;
		_events[i].data.u32 = 0;
		_events[i].data.u64 = 0;
		_events[i].data.ptr = NULL;
	}

	_addr.clear();

	return;
}

void Server::cleanUpClient(std::map<int, Client*>::iterator it) {

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(it->first) + " (client) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	if (it->first != -1) {
		log.debug("Closing fd " + i2a(it->first) + " (client)");
		if (close(it->first) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
	}

	if (it->second != NULL) {
		delete it->second;
		it->second = NULL;
	}

	log.debug("Erasing container entry for above client");
	_clients.erase(it);

}

void Server::cleanUpSocket(std::map<int, const Config::Socket*>::iterator it) {

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(it->first) + " (socket) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	if (it->first != -1) {
		log.debug("Closing fd " + i2a(it->first) + " (socket)");
		if (close(it->first) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
	}

	log.debug("Erasing container entry for above socket");
	_sockets.erase(it);
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
	// if (_epfd != -1 || !_sockfd.empty() || !_clients.empty()) {
	if (_epfd != -1 || !_sockets.empty() || !_clients.empty()) {
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
	if (this != &other) {
		log.debug("Server Copy Assignment Operator called");
	}
	return *this;
}

