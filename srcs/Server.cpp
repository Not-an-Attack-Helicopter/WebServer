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
#include "../incs/SessionManager.hpp"
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
// #include <sys/socket.h>
// #include <sys/epoll.h>
#include <arpa/inet.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <stdexcept>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
// #include <cstring>
// #include <cstdlib>
// #include <cerrno>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
		log.info("Connection(s) closed by the server");
        should_exit = 1;
    }
    return;
}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Server& Server::instance(void) {
	static Server instance;
	return instance;
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

	int fd = -1;
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

	result = socket(sa.sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (result == -1) {
		throw std::runtime_error("socket: " + std::string(strerror(errno)));
	}
	fd = result;

	// ListeningSocket socket;
	// socket.addr = sa;
	// socket.conf = &soc;
	// _sockets[result] = socket;
	// result = setsockopt(_sockets.rbegin()->first, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == -1) {
		throw std::runtime_error("setsockopt: " + std::string(strerror(errno)));
	}

	log.debug("Created server socket listen_" + i2a(fd));

	// result = bind(_sockets.rbegin()->first, (sockaddr*)&sa, sizeof(sa));

	result = bind(fd, (sockaddr*)&sa, sizeof(sa));
	if (result == -1) {
		throw std::runtime_error("bind: " + std::string(strerror(errno)));
	}
// DEBUG BEGIN
	char ipstr[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(sa.sin_family, &sa.sin_addr, ipstr, INET_ADDRSTRLEN)) {
		log.debug("Bound the socket to " + std::string(ipstr) + ":" + i2a(ntohs(sa.sin_port)));
	}
// DEBUG END
	// result = listen(_sockets.rbegin()->first, SOMAXCONN);
	result = listen(fd, SOMAXCONN);
	if (result == -1) {
		throw std::runtime_error("listen: " + std::string(strerror(errno)));
	}

	IPC* ipc = new IPC(LISTEN_SOCKET);
	ipc->fd = fd;
	ipc->addr = sa;
	ipc->conf = &soc;
	_ipcs.push_back(ipc);
	log.error("prepareListeningPort: " + i2a(&ipc) + ":" + i2a(ipc));

	// std::map<int, ListeningSocket>::reverse_iterator it = _sockets.rbegin();
	if (!_setPollInterest(ipc->fd, static_cast<void*>(ipc))) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	log.debug("Now listening on listen_" + i2a(ipc->fd));
	return;
}

void Server::handleEvents(void) {

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	log.info("Awaiting new connection");

	for (;;) {

		log.notice("========================================");
		int n = epoll_wait(_epfd, _events, MAX_EPOLL_EVENTS, EPOLL_WAIT_TIMEOUT_MS);
		// dumpEvents(n, _events);

		switch (n) {

		case -1:
			// throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));
			log.warn("epoll_wait: " + std::string(strerror(errno)));
			break;
// DEBUG BEGIN
		case 0:
			log.debug("Timeout: no events");
			break;

		default:

			// dumpEvents(n, _events);
			log.debug("Total events: " + i2a(n));

			warnHighEventLoad(n, MAX_EPOLL_EVENTS);

			// if (!_sockets.empty()) {
			// 	std::map<int, ListeningSocket>::iterator it = _sockets.begin();
			// 	while (it != _sockets.end()) {
			// 		log.notice("socket_" + i2a(it->first));
			// 		++it;
			// 	}
			// }
			// if (!_clients.empty()) {
			// 	std::map<int, Client*>::iterator it = _clients.begin();
			// 	while (it != _clients.end()) {
			// 		log.notice("client_" + i2a(it->first));
			// 		++it;
			// 	}
			// }
			// if (!_scripts.empty()) {
			// 	std::map<int, int>::iterator it = _scripts.begin();
			// 	while (it != _scripts.end()) {
			// 		log.notice("pipe_" + i2a(it->first));
			// 		++it;
			// 	}
			// }
// DEBUG END

			for (int i = 0; i < n; ++i) {
				// int fd = _events[i].data.fd;
				epoll_event ev = _events[i];
				uint32_t events = ev.events;
				// void* pointer = ev.data.ptr;
				// fd = ev.data.fd;

				IPC* ipc = static_cast<IPC*>(ev.data.ptr);

				std::string type;
				// log.error("ipc->type: " + i2a(ipc->type));
				switch (ipc->type) {
				case LISTEN_SOCKET:
					type = "listen_";
					break;
				case CLIENT_SOCKET:
					type = "client_";
					break;
				case SCRIPT_STD_IO:
					type = "script_";
					break;
				default:
					type = "unknown_";
					break;
				}
				log.debug("Event " + i2a(i + 1) + " " + type + i2a(ipc->fd) + ":");
				if (events & EPOLLIN)		log.debug("\t\tEPOLLIN");
				if (events & EPOLLOUT)		log.debug("\t\tEPOLLOUT");
				if (events & EPOLLERR)		log.debug("\t\tEPOLLERR");
				if (events & EPOLLHUP)		log.debug("\t\tEPOLLHUP");
				if (events & EPOLLRDHUP)	log.debug("\t\tEPOLLRDHUP");
				log.error("handleEvents: " + i2a(&ipc) + ":" + i2a(ipc));

				if (ipc->type == LISTEN_SOCKET) {

					if (events & EPOLLIN) {
						_acceptConnectRequest(ipc->fd, ipc->addr, ipc->conf);
					}

					continue;
				}

				if (ipc->type == CLIENT_SOCKET) {

					if (events & EPOLLERR) {
						_handleSocketError(ipc);
						continue;
					}

					if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) {
						_handleSocketReadEvent(ipc);
						/*
						* _handleSocketReadEvent() may have erased
						* the client from _clients.
						*/
						if (ipc == NULL) {
							continue;
						}
					}

					if (events & EPOLLOUT) {
						_handleSocketWriteEvent(ipc);
					}

					continue;
				}

				if (ipc->type == SCRIPT_STD_IO) {

					if (events & EPOLLERR) {
						_handlePipeError(ipc);
						continue;
					}

					if (events & EPOLLOUT) {
						_handlePipeWriteEvent(ipc);
						/*
						* _handlePipeReadEvent() may erase the pipe.
						*/
						if (ipc == NULL) {
							continue;
						}
					}

					if (events & (EPOLLIN | EPOLLHUP)) {
						_handlePipeReadEvent(ipc);
					}
				}
			}

			break;
		}

			// std::map<int, ListeningSocket>::const_iterator listen_socket = _sockets.find(fd);
			// if (listen_socket != _sockets.end()) {
			// 	if (events & EPOLLIN) {
			// 		_acceptConnectRequest(listen_socket->first, listen_socket->second);
			// 	}
			// 	continue;
			// }

			// std::map<int, Client*>::iterator client_socket = _clients.find(fd);
			// if (client_socket != _clients.end()) {

			// 	if (events & EPOLLERR) {
			// 		_handleSocketError(client_socket);
			// 		continue;
			// 	}

			// 	if (events & EPOLLHUP) {
			// 		_cleanUpClient(client_socket);
			// 		continue;
			// 	}

			// 	if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) {
			// 		_handleSocketReadEvent(client_socket);
			// 		/*
			// 		* _handleSocketReadEvent() may have erased
			// 		* the client from _clients.
			// 		*/
			// 		std::map<int, Client*>::iterator client_it = _clients.find(fd);
			// 		if (client_it == _clients.end()) {
			// 			continue;
			// 		}
			// 	}

			// 	if (events & EPOLLOUT) {
			// 		std::map<int, Client*>::iterator client_it = _clients.find(fd);
			// 		if (client_it != _clients.end()) {
			// 			_handleSocketWriteEvent(client_it);
			// 		}
			// 	}

			// 	continue;
			// }

		// 	std::map<int, int>::iterator script_pipe = _scripts.find(fd);
		// 	if (script_pipe != _scripts.end()) {

		// 		if (events & EPOLLERR) {
		// 			_handlePipeError(script_pipe);
		// 			continue;
		// 		}

		// 		if (events & EPOLLOUT) {
		// 			_handlePipeWriteEvent(script_pipe);
		// 			/*
		// 			* _handlePipeReadEvent() may erase the pipe.
		// 			*/
		// 			std::map<int, int>::iterator script_it = _scripts.find(fd);
		// 			if (script_it == _scripts.end()) {
		// 				continue;
		// 			}
		// 		}

		// 		if (events & (EPOLLIN | EPOLLHUP)) {
		// 			std::map<int, int>::iterator script_it = _scripts.find(fd);
		// 			if (script_it != _scripts.end()) {
		// 				_handlePipeReadEvent(script_pipe);
		// 			}
		// 		}
		// 	}
		// }

		if (should_exit == 1) {
			break;
		}

		const std::time_t now = std::time(NULL);
		if (std::difftime(now, _last_sweep) > EXPIRED_SESSIONS_SWEEP_INTERVAL) {
			session_manager._sweepExpiredSessions(now);
			_last_sweep = now;
		}
		if (std::difftime(now, _last_reap) > STALE_CLIENT_REAP_INTERVAL) {
			_reapStaleClients(now);
			_last_reap = now;
		}
	}
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

bool Server::_setNonblockFlag(int fd) {

	int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		// throw std::runtime_error("fcntl(F_GETFL): " + std::string(strerror(errno)));
		log.error("fcntl(F_GETFL): " + std::string(strerror(errno)));
		return false;
	}

	int status = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (status == -1) {
		// throw std::runtime_error("fcntl(F_SETFL): " + std::string(strerror(errno)));
		log.error("fcntl(F_SETFL): " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setRDWRInterest(int fd, void* ptr) {

	epoll_event e;
	e.data.fd = fd;
	e.data.ptr = ptr;
	e.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	log.error("poll interests: " + i2a(e.events));

	return true;
}

bool Server::_disableSocketIO(int fd, void* ptr) {

	epoll_event e;
	e.data.fd = fd;
	e.data.ptr = ptr;
	e.events = EPOLLRDHUP;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	log.error("poll interests: " + i2a(e.events));

	return true;
}

bool Server::_setPollInterest(int fd, void* ptr, bool is_pipe) {

	log.error("_setPollInterest: " + i2a(ptr) + ":" + i2a(ptr));
	epoll_event e;
	e.events = 0;
	e.data.fd = fd;
	e.data.ptr = ptr;
	if (!is_pipe) {
		e.events = EPOLLIN | EPOLLRDHUP;
	}
	log.error("poll interests: " + i2a(e.events));

	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setRDONLYInterest(int fd, void* ptr, bool is_pipe) {

	epoll_event e;
	e.data.fd = fd;
	e.data.ptr = ptr;
	if (is_pipe) {
		e.events = EPOLLIN | EPOLLHUP;
	} else {
		e.events = EPOLLIN | EPOLLRDHUP;
	}
	log.error("poll interests: " + i2a(e.events));

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setWRONLYInterest(int fd, void* ptr, bool is_pipe) {

	epoll_event e;
	e.data.fd = fd;
	e.data.ptr = ptr;
	if (is_pipe) {
		e.events = EPOLLOUT;
	} else {
		e.events = EPOLLOUT;
	}
	log.error("poll interests: " + i2a(e.events));

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_prepareScriptPipeEnd(int fd, void* ptr, bool is_write_end) {

	log.error("_prepareScriptPipeEnd: " + i2a(&ptr) + ":" + i2a(ptr));

	log.debug("setting poll interest for script_" + i2a(fd));
	if (!_setPollInterest(fd, ptr, true)) {
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	log.debug("setting nonblock flag for script_" + i2a(fd));
	if (!_setNonblockFlag(fd)) {
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	if (is_write_end) {
		log.debug("setting write only interest for script_" + i2a(fd));
		if (!_setWRONLYInterest(fd, ptr, true)) {
			log.error("epoll_ctl: " + std::string(strerror(errno)));
			return false;
		}
	} else {
		log.debug("setting read only interest for script_" + i2a(fd));
		if (!_setRDONLYInterest(fd, ptr, true)) {
			log.error("epoll_ctl: " + std::string(strerror(errno)));
			return false;
		}
	}

	return true;
}

void Server::_acceptConnectRequest(int listen_fd, const sockaddr_in& addr, const Config::Socket* conf) {

	log.info("New connection on socket fd_" + i2a(listen_fd));

	Client* client = new Client(addr, conf);

	int client_fd = accept(listen_fd, &client->getRemoteAddr(), &client->getRemoteAddrlen());
	if (client_fd == -1) {

		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			log.error("accept: " + std::string(strerror(errno)));
		}

		delete client;
		return;
	}

	// _clients[client_fd] = c;
	IPC* client_ipc = new IPC(CLIENT_SOCKET);
	// ipc->type = CLIENT_SOCKET;
	client_ipc->fd = client_fd;
	client_ipc->peer = client;
	std::string type;
	switch (client_ipc->type) {
	case LISTEN_SOCKET:
		type = "listen_socket";
		break;
	case CLIENT_SOCKET:
		type = "client_socket";
		break;
	case SCRIPT_STD_IO:
		type = "script_std_io";
		break;
	default:
		type = "unknown_type";
		break;
	}
	log.error("ipc->type: " + type + " (" + i2a(client_ipc->type) + ")");
	_ipcs.push_back(client_ipc);
	log.error("_acceptConnectRequest: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
	if (!_setNonblockFlag(client_fd)) {
		_cleanUpClient(client_ipc);
		return;
	}
	if (!_setPollInterest(client_fd, static_cast<void*>(client_ipc))) {
		_cleanUpClient(client_ipc);
		return;
	}

	log.info("client_" + i2a(client_fd) + ": endpoint " +
			 client->getRemoteAddress() + ":" + i2a(client->getRemotePort()));

	return;
}

void Server::_handleSocketError(IPC* client_ipc) {

	if (client_ipc == NULL) {
		return;
	}

	int error = 0;
	socklen_t len = sizeof(error);

	if (getsockopt(client_ipc->fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
		log.error("getsockopt(SO_ERROR): " + std::string(strerror(errno)));
	} else if (error != 0) {
		log.error("socket error: " + std::string(strerror(error)));
	}

	// std::map<int, Client*>::iterator it = _clients.find(fd);

	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// }

	_cleanUpClient(client_ipc);
	return;

}

void Server::_handleSocketReadEvent(IPC* client_ipc) {

	// std::map<int, Client*>::iterator it = _clients.find(fd);

	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// 	return false;
	// }

	// int client_fd = client_it->first;
	// Client& client = *client_it->second;

	if (client_ipc == NULL) {
		return;
	}

	if (!client_ipc->peer->bufferSaturated()) {

		ssize_t bytes_received = client_ipc->peer->queueIncomingData(client_ipc->fd);
		// if (!_disableSocketIO(client_fd)) {
		// 	_cleanUpClient(client_it);
		// }

		if (bytes_received < 0) {

			log.warn("recv: client_" + i2a(client_ipc->fd) + ": " + std::string(strerror(errno)));
			_cleanUpClient(client_ipc);
			return;

		} else if (bytes_received == 0) {
			log.info("Connection closed by client fd_" + i2a(client_ipc->fd));
			_cleanUpClient(client_ipc);
			return;
		}
	}

	if (client_ipc->peer->getState() == Client::IDLE ||
		client_ipc->peer->getState() == Client::RECEIVING_HEADERS) {
		std::size_t bytes_consumed = client_ipc->peer->parseDataFromPeer();
		log.debug("client_" + i2a(client_ipc->fd) + ":\tbytes consumed:\t" + i2a(bytes_consumed));
		if (!bytes_consumed) return;

		// if (!capacity_left) {
		// 	if (!_disableSocketIO(client_fd)) {
		// 		_cleanUpClient(client_it);
		// 	}
		// } else {
		// 	if (!_setRDONLYInterest(client_fd)) {
		// 		_cleanUpClient(client_it);
		// 	}
		// }
	}

	if (client_ipc->peer->getState() == Client::RETRIEVING_SESSION) {
		session_manager.retrieveSession(*client_ipc->peer);
	}

	if (client_ipc->peer->getState() == Client::DISPATCHING) {
		// log.notice("client_" + i2a(client_fd) + " state: DISPATCHING");
		dispatcher.handleRequest(*client_ipc->peer);
	}

	log.error("CLIENT STATE: " + i2a(client_ipc->peer->getState()));
	log.error("CGI PROCESS: " + i2a(&client_ipc->peer->cgi_process) + ":" + i2a(client_ipc->peer->cgi_process));
	log.error("CGI STD_IN: " + i2a(&client_ipc->script_in) + ":" + i2a(client_ipc->script_in));
	log.error("CGI STD_OUT: " + i2a(&client_ipc->script_out) + ":" + i2a(client_ipc->script_out));
	// TEST register pipe end and add it to _scripts
	if (client_ipc->peer->cgi_process != NULL) {

		if (client_ipc->peer->getState() == Client::RECEIVING_BODY) {

			if (client_ipc->script_in == NULL && client_ipc->peer->cgi_process->wantsWrite()) {
				IPC* script_ipc = new IPC(SCRIPT_STD_IO);
				log.error("_handleSocketReadEvent: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
				script_ipc->fd = client_ipc->peer->cgi_process->stdinFd();
				// script_ipc->peer = client_ipc->peer;
				script_ipc->client_ipc = client_ipc;
				_ipcs.push_back(script_ipc);
				if (!_prepareScriptPipeEnd(script_ipc->fd, static_cast<void*>(script_ipc), true)) {
					_handlePipeError(script_ipc);
					return;
				}
				client_ipc->script_in = script_ipc;
				log.error("CREATED CGI script_" + i2a(script_ipc->fd) + " for client_" + i2a(client_ipc->fd));
			}

			if (client_ipc->script_out == NULL && client_ipc->peer->cgi_process->wantsRead()) {
				IPC* script_ipc = new IPC(SCRIPT_STD_IO);
				log.error("_handleSocketReadEvent: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
				script_ipc->fd = client_ipc->peer->cgi_process->stdoutFd();
				// script_ipc->peer = client_ipc->peer;
				script_ipc->client_ipc = client_ipc;
				_ipcs.push_back(script_ipc);
				if (!_prepareScriptPipeEnd(script_ipc->fd, static_cast<void*>(script_ipc))) {
					_handlePipeError(script_ipc);
					return;
				}
				client_ipc->script_out = script_ipc;
				log.error("CREATED CGI script_" + i2a(script_ipc->fd) + " for client_" + i2a(client_ipc->fd));
			}

			// Returning here as we have to wait for pipe readyness to start writing to std_in and reading from std_out.
			// _handlePipeWriteEvent() and _handlePipeReadEvent() take it from here
			log.info("2-way communication with CGI process");
			return;
		}

		if (client_ipc->peer->getState() == Client::AWAITING_CGI_OUTPUT) {

			client_ipc->peer->popRequest();
			client_ipc->peer->pushRequest();
			client_ipc->peer->cgi_process->closeStdin();

			// if (client_ipc->peer->cgi_process->wantsWrite()) {
			// 	close(client_ipc->peer->cgi_process->stdinFd());
			// }

			if (client_ipc->script_out == NULL && client_ipc->peer->cgi_process->wantsRead()) {
				IPC* script_ipc = new IPC(SCRIPT_STD_IO);
				log.error("_handleSocketReadEvent: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
				log.error("fd: " + i2a(client_ipc->peer->cgi_process->stdoutFd()));
				script_ipc->fd = client_ipc->peer->cgi_process->stdoutFd();
				// script_ipc->peer = client_ipc->peer;
				script_ipc->client_ipc = client_ipc;
				_ipcs.push_back(script_ipc);
				if (!_prepareScriptPipeEnd(script_ipc->fd, static_cast<void*>(script_ipc))) {
					_handlePipeError(script_ipc);
					return;
				}
				client_ipc->script_out = script_ipc;
				log.error("CREATED CGI script_" + i2a(script_ipc->fd) + " for client_fd=" + i2a(client_ipc->fd));
			}

			// Returning here as we have to wait for pipe readyness to start reading from std_out.
			// _handlePipeReadEvent() takes it from here
			log.info("1-way communication with CGI process");
			return;
		}

		return;
	}

	if (client_ipc->peer->getState() == Client::RECEIVING_BODY) {
		// log.notice("client_" + i2a(client_fd) + " state: RECEIVING_BODY");
		std::size_t bytes_consumed = client_ipc->peer->parseDataFromPeer();
		log.debug("client_" + i2a(client_ipc->fd) + ":\tbytes consumed:\t" + i2a(bytes_consumed));
		if (!bytes_consumed) return;

		// if (!capacity_left) {
		// 	if (!_disableSocketIO(client_fd)) {
		// 		_cleanUpClient(client_it);
		// 	}
		// } else {
		// 	if (!_setRDONLYInterest(client_fd)) {
		// 		_cleanUpClient(client_it);
		// 	}
		// }
	}

	if (client_ipc->peer->getState() == Client::PREPARING_RESPONSE) {
		// log.notice("client_" + i2a(client_fd) + " state: PREPARING_RESPONSE");
		dispatcher.handleRequest(*client_ipc->peer);
	}

	if (client_ipc->peer->getState() == Client::PENDING_RESPONSE) {
		// log.notice("client_" + i2a(client_fd) + " state: PENDING_RESPONSE");
		client_ipc->peer->popRequest();
		client_ipc->peer->pushRequest();

		if (client_ipc->peer->blockedFromReceiving()) {
			if (!_setWRONLYInterest(client_ipc->fd, static_cast<void*>(client_ipc))) {
				_cleanUpClient(client_ipc);
			}
		} else {
			if (!_setRDWRInterest(client_ipc->fd, static_cast<void*>(client_ipc))) {
				_cleanUpClient(client_ipc);
			}
		}
	}

	return;
}


void Server::_handleSocketWriteEvent(IPC* client_ipc) {

	// std::map<int, Client*>::iterator it = _clients.find(fd);
 //
	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// 	return;
	// }

	// int fd = it->first;
	// Client& client = *it->second;

	if (client_ipc == NULL) {
		return;
	}

	if (client_ipc->peer->getState() == Client::AWAITING_CGI_OUTPUT) return;

	// if (client.getState() == Client::CONCLUDED ||
	// 	client.getState() == Client::REJECTED) {
	// 	return;
	// }

	if (client_ipc->peer->getState() == Client::PENDING_RESPONSE) {
		client_ipc->peer->queueOutgoingData();
		client_ipc->peer->popProcess();
		client_ipc->peer->popResponse();
		client_ipc->peer->pushResponse();
	}

	if (client_ipc->peer->getState() == Client::SENDING_HEADERS ||
		client_ipc->peer->getState() == Client::SENDING_BODY) {
		client_ipc->peer->sendDataToTCPPeer(client_ipc->fd);
	}

	switch (client_ipc->peer->getState()) {

	case Client::IDLE:
		if (!_setRDONLYInterest(client_ipc->fd, static_cast<void*>(client_ipc))) {
			_cleanUpClient(client_ipc);
		}
		break;
	case Client::ERROR:
		_cleanUpClient(client_ipc);
		break;
	case Client::REJECTED:
		if (!_disableSocketIO(client_ipc->fd, static_cast<void*>(client_ipc))) {
			_cleanUpClient(client_ipc);
		}
		break;
	case Client::CONCLUDED:
		_cleanUpClient(client_ipc);
	default:
		break;

	}

	return;
}

void Server::_handlePipeError(IPC* script_ipc) {

	// int client_fd = -1;
	// std::map<Client*, int>::const_iterator ti = _reverse.find(script_it->second);
	// if (ti != _reverse.end()) {
	// 	client_fd = ti->second;
	// }

	// int client_fd = script_it->second;
	// std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	// if (client_it == _clients.end()) {
	// 	return;
	// }
	// Client& client = *client_it->second;

	if (script_ipc == NULL) {
		return;
	}

	Client& client = *script_ipc->client_ipc->peer;

	// _cleanUpPipeEnd(script_ipc);
	client.cgi_process->forceKill();
	dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
								  client.getCurrentRequest().resolved.location,
								  client.getCurrentRequest().headers_only,
								  client.getCurrentResponse());
	client.setState(Client::PENDING_RESPONSE);
	log.debug("client_" + i2a(script_ipc->client_ipc->fd) + ": state set to PENDING_RESPONSE");
	client.popProcess();
	client.popRequest();
	client.pushRequest();
	if (!_setWRONLYInterest(script_ipc->client_ipc->fd, static_cast<void*>(script_ipc->client_ipc))) {
		_cleanUpClient(script_ipc->client_ipc);
		return;
	}
	client.markForTermination();

	_cleanUpPipeEnd(script_ipc);
	return;
}

void Server::_handlePipeWriteEvent(IPC* script_ipc) {

	// int fd = it->first;
	// int client_fd = -1;
	// std::map<Client*, int>::const_iterator ti = _reverse.find(script_it->second);
	// if (ti != _reverse.end()) {
	// 	client_fd = ti->second;
	// }
	// Client& client = *script_it->second;

	// if (client.getState() != Client::RECEIVING_BODY) {
	// 	return;
	// }

	// if (!client.cgi_process->wantsWrite()) {
	// 	client.setState(Client::PREPARING_RESPONSE);
	// }

	// Client* client = NULL;
	// int client_fd = script_it->second;
	// std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	// if (client_it == _clients.end()) {
	// 	return;
	// }
	// Client& client = *client_it->second;

	// if (client_it != _clients.end()) {
	// 	client = client_it->second;
	// }
	// if (client == NULL) return;

	if (script_ipc == NULL) {
		return;
	}

	Client& client = *script_ipc->client_ipc->peer;

	std::size_t bytes_consumed = client.parseDataFromPeer();
	// log.debug("client_" + i2a(client_fd) + ":\tbytes written:\t" + i2a(bytes_consumed));
	if (!bytes_consumed) return;

	// if (!capacity_left) {
	// 	if (!_disableSocketIO(client_fd)) {
	// 		_cleanUpClient(client_it);
	// 	}
	// } else {
	// 	if (!_setRDONLYInterest(client_fd)) {
	// 		_cleanUpClient(client_it);
	// 	}
	// }

	// if (bytes_consumed == 0) {
	// 	if (!_disableSocketIO(client_fd)) {
	// 		_cleanUpClient(client_it);
	// 	}
	// } else {
	// 	if (!_setRDONLYInterest(client_fd)) {
	// 		_cleanUpClient(client_it);
	// 	}
	// }

	// if (client.getState() == Client::PREPARING_RESPONSE) {
	// 	_cleanUpPipeEnd(script_it);
	// 	// TODO decide:
	// 	// calling dispatcher wouldn't be needed if _state
	// 	// was set to AWAITING_CGI_OUTPUT at end of
	// 	dispatcher.handleRequest(client);
	// }

	if (client.getState() == Client::AWAITING_CGI_OUTPUT) {

		log.info("client_" + i2a(script_ipc->client_ipc->fd) +
				 ": CGI process: finished providing input, now awaiting output");

		_cleanUpPipeEnd(script_ipc);
		// script_ipc->client_ipc->script_out = NULL;
		client.popRequest();
		client.pushRequest();

		// if (client.cgi_process->wantsRead()) {
		// 	int std_out = client.cgi_process->stdoutFd();
		// 	if (_scripts.find(std_out) == _scripts.end()) {
		// 		_scripts[std_out] = client_fd;
		// 		if (!_prepareScriptPipeEnd(std_out)) {
		// 			std::map<int, int>::iterator it = _scripts.find(std_out);
		// 			if (it != _scripts.end()) {
		// 				_handlePipeError(it);
		// 			}
		// 		}
		// 	}
		// }
	}

	return;
}

void Server::_handlePipeReadEvent(IPC* script_ipc) {

	// int fd = script_it->first;
	// int client_fd = -1;
	// std::map<Client*, int>::const_iterator ti = _reverse.find(script_it->second);
	// if (ti != _reverse.end()) {
	// 	client_fd = ti->second;
	// }
	// Client& client = *script_it->second;

	// Client* client = NULL;
	// int std_out = script_it->first;
	// int client_fd = script_it->second;
	// std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	// if (client_it == _clients.end()) {
	// 	return;
	// }
	// Client& client = *client_it->second;

	// if (client.getState() != Client::AWAITING_CGI_OUTPUT) {
	// 	return;
	// }

	// if (!client.cgi_process->wantsRead()) {
	// 	client.cgi_process->buildResponse(client.getCurrentResponse(),
	// 									  client.getCurrentRequest().headers_only);
	// 	client.setState(Client::PENDING_RESPONSE);
	// }

	if (script_ipc == NULL) {
		return;
	}

	// Client& client = *script_ipc->client_ipc->peer;
	log.error("_handlePipeReadEvent: " + i2a(&script_ipc->client_ipc) + ":" + i2a(script_ipc->client_ipc));

	ssize_t bytes_read = script_ipc->client_ipc->peer->cgi_process->queueIncomingData(script_ipc->fd);
	// log.debug("script_" + i2a(std_out) + ":\tbytes read:\t" + i2a(bytes_read));

	if (bytes_read < 0) {

		log.warn("read: script_" + i2a(script_ipc->fd) + ": " + std::string(strerror(errno)));
		// _cleanUpPipeEnd(it);
		// dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
		// 							  client.getCurrentRequest().resolved.location,
		// 							  client.getCurrentRequest().headers_only,
		// 							  client.getCurrentResponse());
		// client.setState(Client::PENDING_RESPONSE);
		// log.debug("client_" + i2a(client_fd) + ": state set to PENDING_RESPONSE");
		// client.popRequest();
		// if (!_setWRONLYInterest(client_fd)) {
		// 	_cleanUpClient(it);
		// 	return;
		// }
		// client.markForTermination();
		// return;
		_handlePipeError(script_ipc);

	} else if (bytes_read == 0) {

		log.info("script_" + i2a(script_ipc->fd) + ": full response received");
		// _cleanUpPipeEnd(script_ipc);
		// // script_ipc->client_ipc->script_in = NULL;
		bool is_dead = script_ipc->client_ipc->peer->cgi_process->tryReap();
		is_dead ? log.error("HE'S DEAD") : log.error("HE'S ALIVE!");
		if (!is_dead) {
			script_ipc->client_ipc->peer->cgi_process->forceKill();
			is_dead = script_ipc->client_ipc->peer->cgi_process->tryReap();
			is_dead ? log.error("HE'S DEAD") : log.error("HE'S STILL ALIVE!");
		}
		// if (script_ipc->client_ipc->peer->blockedFromReceiving()) {
		// 	log.error("CLIENT BLOCKED FROM RECEIVING: " + i2a(script_ipc->client_ipc->fd));
		// } else {
		// 	log.error("CLIENT NOT BLOCKED FROM RECEIVING: " + i2a(script_ipc->client_ipc->fd));
		// }
		script_ipc->client_ipc->peer->cgi_process->buildResponse(script_ipc->client_ipc->peer->getCurrentResponse(),
																 script_ipc->client_ipc->peer->getCurrentRequest().headers_only);
		// log.error("_handlePipeReadEvent: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
		// log.error("_handlePipeReadEvent: " + i2a(&script_ipc->client_ipc) + ":" + i2a(script_ipc->client_ipc));
		// log.error("CLIENT FD: " + i2a(script_ipc->client_ipc->fd));
		// if (script_ipc->client_ipc->peer->blockedFromReceiving()) {
		// 	log.error("CLIENT BLOCKED FROM RECEIVING: " + i2a(script_ipc->client_ipc->fd));
		// 	if (!_setWRONLYInterest(script_ipc->client_ipc->fd, static_cast<void*>(script_ipc->client_ipc))) {
		// 		_cleanUpClient(script_ipc->client_ipc);
		// 	}
		// } else {
		// 	log.error("CLIENT NOT BLOCKED FROM RECEIVING: " + i2a(script_ipc->client_ipc->fd));
		// 	if (!_setRDWRInterest(script_ipc->client_ipc->fd, static_cast<void*>(script_ipc->client_ipc))) {
		// 		_cleanUpClient(script_ipc->client_ipc);
		// 	}
		// }
		if (!_setWRONLYInterest(script_ipc->client_ipc->fd, static_cast<void*>(script_ipc->client_ipc))) {
			_cleanUpClient(script_ipc->client_ipc);
		} else {
			script_ipc->client_ipc->peer->setState(Client::PENDING_RESPONSE);
		}
		_cleanUpPipeEnd(script_ipc);
		// script_ipc->client_ipc->script_in = NULL;

		// }

		// if (client.getState() == Client::PENDING_RESPONSE) {
		// 	// log.notice("client_" + i2a(client_fd) + " state: PENDING_RESPONSE");
		// 	client.popRequest();
		// 	client.pushRequest();

		// 	if (client.blockedFromReceiving()) {
		// 		if (!_setWRONLYInterest(client_fd)) {
		// 			_cleanUpClient(client_it);
		// 		}
		// 	} else {
		// 		if (!_setRDWRInterest(client_fd)) {
		// 			_cleanUpClient(client_it);
		// 		}
		// 	}
		// }

		return;

	} else {

		// log.notice("some bytes read from pipe");
		script_ipc->client_ipc->peer->updateTimeStamp();
		// TEST have CGIProcess consume the data in the buffer
		try {
			// log.notice("consuming bytes from pipe");
			script_ipc->client_ipc->peer->cgi_process->consumeAvailableOutput();
		} catch (std::exception& e) {
			log.warn("read: script_" + i2a(script_ipc->fd) + ": " + std::string(e.what()));
			// _cleanUpPipeEnd(it);
			// dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
			// 							  client.getCurrentRequest().resolved.location,
			// 							  client.getCurrentRequest().headers_only,
			// 							  client.getCurrentResponse());
			// client.setState(Client::PENDING_RESPONSE);
			// log.debug("client_" + i2a(client_fd) + ": state set to PENDING_RESPONSE");
			// client.popRequest();
			// if (!_setWRONLYInterest(client_fd)) {
			// 	_cleanUpClient(it);
			// 	return;
			// }
			// client.markForTermination();
			// return;
			_handlePipeError(script_ipc);
		}
	}

	return;
}

// void Server::_handlePipeEOFEvent(std::map<int, Client*>::iterator it) {
//
// 	log.info("EOF received via fd_" + i2a(it->first));
// 	int client_fd = -1;
// 	std::map<Client*, int>::iterator ti = _reverse.find(it->second);
// 	if (ti != _reverse.end()) {
// 		client_fd = ti->second;
// 	}
// 	Client& client = *it->second;
//
// 	_cleanUpPipeEnd(it);
// 	client.cgi_process->buildResponse(client.getCurrentResponse(),
// 									  client.getCurrentRequest().headers_only);
// 	client.cgi_process->tryReap();
// 	client.setState(Client::PENDING_RESPONSE);
// 	if (!_setWRONLYInterest(client_fd)) {
// 		_cleanUpClient(it);
// 		return;
//    }
// }

void Server::_reapStaleClients(const std::time_t now) {

	// std::map<int, Client*>::iterator immediate;
	// std::map<int, Client*>::iterator it = _clients.begin();
	// while (it != _clients.end()) {
		// immediate = it;
		// ++it;
	for (std::size_t i = 0; i < _ipcs.size(); ++i) {

		if (_ipcs[i]->type == CLIENT_SOCKET) {

		// if (immediate->second->isTimedOut(now)) {
			if (_ipcs[i]->peer->isTimedOut(now)) {

			// int fd = immediate->first;
			// Client& client = *immediate->second;
				int client_fd = _ipcs[i]->fd;
				Client& client = *_ipcs[i]->peer;
	// DEBUG BEGIN
				log.debug("client_" + i2a(client_fd)
				+ " idle time: " + i2a(client.getIdleTime()) + "s");
	// DEBUG END
				log.warn("client_" + i2a(client_fd) + " timed out");

				if (client.getState() == Client::RECEIVING_HEADERS) {
					dispatcher.buildErrorResponse(REQUEST_TIMEOUT,
												client.getCurrentRequest().resolved.location,
												client.getCurrentRequest().headers_only,
												client.getCurrentResponse());
					client.setState(Client::PENDING_RESPONSE);
					log.debug("client_" + i2a(client_fd) + ": state set to PENDING_RESPONSE");
					client.popRequest();
					client.pushRequest();
					if (_setWRONLYInterest(client_fd, static_cast<void*>(_ipcs[i]))) {
						client.markForTermination();
						continue;
					}
				}
				// _cleanUpClient(immediate);
				_cleanUpClient(_ipcs[i]);
			}
// DEBUG BEGIN
			// if (_clients.empty()) {
			// 	log.info("All clients disconnected");
			// }
// DEBUG END
		}
	}
}

void Server::_cleanUpAllRessources(void) {

	// if (!_scripts.empty()) {

	// 	std::map<int, int>::iterator immediate;
	// 	std::map<int, int>::iterator it = _scripts.begin();

	// 	while (it != _scripts.end()) {
	// 		immediate = it;
	// 		++it;
	// 		log.debug("Cleaning up pipe_" + i2a(immediate->first));
	// 		_cleanUpPipeEnd(immediate);
	// 	}
	// }
	// _scripts.clear();

	// if (!_clients.empty()) {

	// 	std::map<int, Client*>::iterator immediate;
	// 	std::map<int, Client*>::iterator it = _clients.begin();

	// 	while (it != _clients.end()) {
	// 		immediate = it;
	// 		++it;
	// 		log.debug("Cleaning up client_" + i2a(immediate->first));
	// 		_cleanUpClient(immediate);
	// 	}

	// }
	// _clients.clear();

	// if (!_sockets.empty()) {

	// 	std::map<int, ListeningSocket>::iterator immediate;
	// 	std::map<int, ListeningSocket>::iterator it = _sockets.begin();

	// 	while (it != _sockets.end()) {
	// 		immediate = it;
	// 		++it;
	// 		log.debug("Cleaning up socket_" + i2a(immediate->first));
	// 		_cleanUpSocket(immediate);
	// 	}

	// }
	// _sockets.clear();

	for (int i = _ipcs.size() - 1; i > -1; --i) {
	// for (std::size_t i = 0; i < _ipcs.size(); ++i) {

		log.error(i2a(i) + ": " + i2a(_ipcs[i]->fd));

		if (_ipcs[i]->type == LISTEN_SOCKET) {
			// _ipcs.erase(_ipcs.begin() + i);
			_cleanUpSocket(_ipcs[i]);
			continue;
		}

		if (_ipcs[i]->type == CLIENT_SOCKET) {
			// _ipcs.erase(_ipcs.begin() + i);
			_cleanUpClient(_ipcs[i]);
			continue;
		}

		if (_ipcs[i]->type == SCRIPT_STD_IO) {
			// _ipcs.erase(_ipcs.begin() + i);
			_cleanUpPipeEnd(_ipcs[i]);
			continue;
		}

	}

	if (_epfd != -1) {

		log.debug("Closing fd " + i2a(_epfd) + " (epoll instance epfd)");
		if (close(_epfd) == -1) {
			log.warn("error during cleanup: close: " + std::string(strerror(errno)));
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

	return;
}

void Server::_cleanUpPipeEnd(IPC* script_ipc) {

	if (script_ipc == NULL) {
		return;
	}

	log.error("CLOSING CGI PIPE script_" + i2a(script_ipc->fd) + " for client_" + i2a(script_ipc->client_ipc->fd));

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(script_ipc->fd) + " (script pipe end) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, script_ipc->fd, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	// if (script_ipc->fd != -1) {
	// 	log.debug("Closing fd " + i2a(script_ipc->fd) + " (script pipe end)");
	// 	if (close(script_ipc->fd) == -1) {
	// 		log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
	// 	}
	// 	script_ipc->fd = -1;
	// }

	if (script_ipc->client_ipc->script_in == script_ipc) {
		script_ipc->client_ipc->script_in = NULL;
		log.debug("Closing fd " + i2a(script_ipc->client_ipc->peer->cgi_process->stdinFd()) + " (script pipe end)");
		script_ipc->client_ipc->peer->cgi_process->closeStdin();
		script_ipc->fd = -1;
	}

	if (script_ipc->client_ipc->script_out == script_ipc) {
		script_ipc->client_ipc->script_out = NULL;
		log.debug("Closing fd " + i2a(script_ipc->client_ipc->peer->cgi_process->stdoutFd()) + " (script pipe end)");
		script_ipc->client_ipc->peer->cgi_process->closeStdout();
		script_ipc->fd = -1;
	}

	log.debug("Erasing container entry for above script pipe end");
	for (std::size_t i = 0; i < _ipcs.size(); ++i) {
		if (_ipcs[i] == script_ipc) {
			log.error("CRACK!");
			_ipcs.erase(_ipcs.begin() + i);
		}
	}

	delete script_ipc;
	log.error("_cleanUpPipeEnd: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
	script_ipc = NULL;
	log.error("_cleanUpPipeEnd: " + i2a(&script_ipc) + ":" + i2a(script_ipc));
	return;
}

void Server::_cleanUpClient(IPC* client_ipc) {

	if (client_ipc == NULL) {
		log.warn("client_ipc was NULL");
		return;
	}

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(client_ipc->fd) + " (client) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, client_ipc->fd, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	if (client_ipc->fd != -1) {
		log.debug("Closing fd " + i2a(client_ipc->fd) + " (client)");
		if (close(client_ipc->fd) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
		client_ipc->fd = -1;
	}

	// if (client_ipc->peer->cgi_process != NULL) {
 //
	// 	// int stdin_fd = client_ipc->peer->cgi_process->stdinFd();
	// 	// int stdout_fd = client_ipc->peer->cgi_process->stdoutFd();
 //
	// // 	std::map<int, int>::iterator stdin_it = _scripts.find(stdin_fd);
	// // 	if (stdin_it != _scripts.end()) {
	// // 		_cleanUpPipeEnd(stdin_it);
	// // 	}
	// // 	std::map<int, int>::iterator stdout_it = _scripts.find(stdout_fd);
	// // 	if (stdout_it != _scripts.end()) {
	// // 		_cleanUpPipeEnd(stdout_it);
	// // 	}
	// // }
 //
	// 	if (client_ipc->script_in != NULL) {
	// 		log.error("DING!");
	// 		log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
	// 		log.error("_cleanUpClient: " + i2a(&client_ipc->script_in) + ":" + i2a(client_ipc->script_in));
	// 		_cleanUpPipeEnd(client_ipc->script_in);
	// 	}
 //
	// 	if (client_ipc->script_out != NULL) {
	// 		log.error("DONG!");
	// 		log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
	// 		log.error("_cleanUpClient: " + i2a(&client_ipc->script_out) + ":" + i2a(client_ipc->script_out));
	// 		_cleanUpPipeEnd(client_ipc->script_out);
	// 	}
	// }

	if (client_ipc->script_in != NULL) {
		log.error("DING!");
		log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
		log.error("_cleanUpClient: " + i2a(&client_ipc->script_in) + ":" + i2a(client_ipc->script_in));
		_cleanUpPipeEnd(client_ipc->script_in);
	}

	if (client_ipc->script_out != NULL) {
		log.error("DONG!");
		log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
		log.error("_cleanUpClient: " + i2a(&client_ipc->script_out) + ":" + i2a(client_ipc->script_out));
		_cleanUpPipeEnd(client_ipc->script_out);
	}

	delete client_ipc->peer;

	log.debug("Erasing container entry for above client");
	for (std::size_t i = 0; i < _ipcs.size(); ++i) {
		if (_ipcs[i] == client_ipc) {
			log.error("BANG!");
			_ipcs.erase(_ipcs.begin() + i);
		}
	}

	delete client_ipc;
	log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
	client_ipc = NULL;
	log.error("_cleanUpClient: " + i2a(&client_ipc) + ":" + i2a(client_ipc));
	return;
}

void Server::_cleanUpSocket(IPC* socket_ipc) {

	// int socket_fd = it->first;

	if (socket_ipc == NULL) {
		return;
	}

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(socket_ipc->fd) + " (socket) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, socket_ipc->fd, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	if (socket_ipc->fd != -1) {
		log.debug("Closing fd " + i2a(socket_ipc->fd) + " (socket)");
		if (close(socket_ipc->fd) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
		socket_ipc->fd = -1;
	}

	log.debug("Erasing container entry for above socket");
	// _sockets.erase(it);
	for (std::size_t i = 0; i < _ipcs.size(); ++i) {
		if (_ipcs[i] == socket_ipc) {
			log.error("BOOM!");
			_ipcs.erase(_ipcs.begin() + i);
		// log.error(i2a(_ipcs.begin() + i));
		}
	}

	delete socket_ipc;
	log.error("_cleanUpSocket: " + i2a(&socket_ipc) + ":" + i2a(socket_ipc));
	socket_ipc = NULL;
	log.error("_cleanUpSocket: " + i2a(&socket_ipc) + ":" + i2a(socket_ipc));
	return;
}

/*	@brief Constructor	*/
Server::Server(void) {
	log.debug("Server Constructor called");
	const std::time_t now = std::time(NULL);
	_last_sweep = now;
	_last_reap = now;
	_epfd = -1;
	return;
}

/*	@brief Destructor	*/
Server::~Server(void) {
	log.debug("Server Destructor called");
	// if (_epfd != -1 || !_sockets.empty() || !_clients.empty()) {
	if (_epfd != -1 || !_ipcs.empty()) {
		_cleanUpAllRessources();
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
