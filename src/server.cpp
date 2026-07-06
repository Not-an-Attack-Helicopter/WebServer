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

#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/colors.hpp"
#include "../includes/types.hpp"
#include "../includes/requesthandler.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Server::Server(void) {
	std::cerr	<< DEBUG << "Server Constructor called" << RESET
				<< std::endl;
	_epfd = -1;
	_stop = false;
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

void Server::setConfig(const Config& cfg) {
	_config = cfg;
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

void Server::prepareListeningPort(const std::string& address, unsigned short port,
		const ServerConfig* srv_cfg) {
	int opt = 1;
	int result = 0;
	sockaddr_in sa;
	std::memset(&sa, 0, sizeof(sa));
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
	_sockfd.push_back(result);
	if (srv_cfg) {
		_listen_configs[result] = srv_cfg;
	}
	result = setsockopt(_sockfd.back(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == -1) {
		throw SetOptionException();
	}
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
	while (true) {
		int nfds = epoll_wait(_epfd, _events, MAXEVENTS, -1);
		if (nfds == -1) {
			throw EventPollingException();
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
		if (_clients.empty()) {
			std::cout	<< INFO << "All clients disconnected"
						<< RESET << std::endl;
			// break;
		}
		if (_stop == true) {
			break;
		}
	}
	return;
}

void Server::acceptConnectRequest(int socket_fd) {
	std::cout	<< INFO << "New connection on socket fd: " << socket_fd
				<< RESET << std::endl;

	sockaddr_in client_addr;
	socklen_t   client_len = sizeof(client_addr);
	int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd == -1) {
		throw AcceptException();
	}

	const ServerConfig* srv_cfg = NULL;
	std::map<int, const ServerConfig*>::iterator cfg_it = _listen_configs.find(socket_fd);
	if (cfg_it != _listen_configs.end()) {
		srv_cfg = cfg_it->second;
	}

	Client* c = new Client(client_fd, srv_cfg);
	_clients[client_fd] = c;
	setNonblockFlag(client_fd);
	setReadInterest(client_fd);

	char ipstr[INET_ADDRSTRLEN] = {0};
	inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, INET_ADDRSTRLEN);
	std::cout	<< INFO << "Client #" << client_fd
				<< ", endpoint " << ipstr << ":" << ntohs(client_addr.sin_port)
				<< RESET << std::endl;
	// DEBUG We don't care about potential DoS attack vectors here
	pid_t pid = fork();
	const char* argv[] = {"echo", "Hello from the child process!", NULL};
	switch(pid) {
	case -1:
		std::cout << ERROR << "Error: fork: " << strerror(errno) << RESET << std::endl;
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
			std::cout << ERROR << "Oh no! " << strerror(errno) << RESET << std::endl;
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
		throw MissingClientException();
	}

	Client* client = it->second;
	if (!client->read_data()) {
		std::cerr	<< INFO << "Connection closed by client #" << fd
					<< RESET << std::endl;
		cleanUpClient(it);
		return;
	}

	HTTPRequest& req = client->getRequest();
	if (req.parse(client->getRecvBuf())) {
		if (client->getServer()) {
			RequestHandler handler(*client->getServer(), req);
			HTTPResponse res = handler.handler();
			client->setSendBuf(res.serialize());
			addWriteInterest(fd);
		} else {
			HTTPResponse res;
			res.setStatus(500, "Internal Server Error");
			std::ifstream in("HTML_Files/error_pages/500.html", std::ios::binary);
			if (in.is_open()) {
				std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
				in.close();
				res.setBody(body, "text/html");
			} else {
				res.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>", "text/html");
			}
			client->setSendBuf(res.serialize());
			addWriteInterest(fd);
		}
	}
	return;
}

void Server::handleWriteEvent(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end() || it->second == NULL) {
		throw MissingClientException();
	}

	Client* client = it->second;
	if (!client->write_data()) {
		// Write error — close connection
		std::cerr	<< INFO << "Write error, closing client #" << fd
					<< RESET << std::endl;
		cleanUpClient(it);
		return;
	}

	if (client->isSendBufEmpty()) {
		removeWriteInterest(fd);
		// Keep-alive: reset client state for next request
		client->reset();
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

const char* Server::SetOptionException::what(void) const throw () {
	// return "SetOptionException\n";
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

const char* Server::MissingClientException::what(void) const throw () {
	// return "MissingClientException\n";
	std::cerr << ERROR << "Error: client lookup: ";
	return ENOCLNT;
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

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Server::Server(const Server& other) {
	std::cerr	<< DEBUG << "Server Copy Constructor called" << RESET
				<< std::endl;
	*this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Server& Server::operator = (const Server& other) {
	std::cerr	<< DEBUG << "Server Copy Assignment Operator called" << RESET
				<< std::endl;
	if (this != &other) {}
	return *this;
}
