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

bool Server::_setRDWRInterest(int fd) {

	epoll_event e;
	e.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
	e.data.fd = fd;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_dropWriteInterest(int fd) {

	epoll_event e;
	e.events = EPOLLRDHUP;
	e.data.fd = fd;

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setPollInterest(int fd, bool is_pipe) {

	epoll_event e;
	e.data.fd = fd;
	e.events = 0;
	if (!is_pipe) {
		e.events = EPOLLIN | EPOLLRDHUP;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setRDONLYInterest(int fd, bool is_pipe) {

	epoll_event e;
	e.data.fd = fd;
	if (is_pipe) {
		e.events = EPOLLIN;
	} else {
		e.events = EPOLLIN | EPOLLRDHUP;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_setWRONLYInterest(int fd, bool is_pipe) {

	epoll_event e;
	e.data.fd = fd;
	if (is_pipe) {
		e.events = EPOLLOUT;
	} else {
		e.events = EPOLLOUT | EPOLLRDHUP;
	}

	int status = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &e);
	if (status == -1) {
		// throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}

	return true;
}

bool Server::_prepareScriptPipeEnd(int fd, bool is_read_end) {

	log.debug("setting poll interest for " + i2a(fd));
	if (!_setPollInterest(fd, true)) {
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	log.debug("setting nonblock flag for " + i2a(fd));
	if (!_setNonblockFlag(fd)) {
		log.error("epoll_ctl: " + std::string(strerror(errno)));
		return false;
	}
	if (is_read_end) {
		log.debug("setting read only interest for " + i2a(fd));
		if (!_setRDONLYInterest(fd, true)) {
			log.error("epoll_ctl: " + std::string(strerror(errno)));
			return false;
		}
	} else {
		log.debug("setting write only interest for " + i2a(fd));
		if (!_setWRONLYInterest(fd, true)) {
			log.error("epoll_ctl: " + std::string(strerror(errno)));
			return false;
		}
	}

	return true;
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

	// _addr.push_back(sa);

	result = socket(sa.sin_family, SOCK_STREAM | O_NONBLOCK, 0);
	if (result == -1) {
		throw std::runtime_error("socket: " + std::string(strerror(errno)));
	}

	const ListeningSocket socket = { sa, &soc };
	_sockets[result] = socket;
	// _sockets[result] = &soc;

	result = setsockopt(_sockets.rbegin()->first, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == -1) {
		throw std::runtime_error("setsockopt: " + std::string(strerror(errno)));
	}

	log.debug("Created server socket fd_" + i2a(_sockets.rbegin()->first) + "(listen_fd)");

	result = bind(_sockets.rbegin()->first, (sockaddr*)&sa, sizeof(sa));
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

	if (!_setPollInterest(_sockets.rbegin()->first)) {
		throw std::runtime_error("epoll_ctl: " + std::string(strerror(errno)));
	}

	log.debug("Now listening on listen_fd fd_" + i2a(_sockets.rbegin()->first));
	return;
}

void Server::handleEvents(void) {

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	log.info("Awaiting new connection");

	for (;;) {

		int nfds = epoll_wait(_epfd, _events, MAX_EPOLL_EVENTS, EPOLL_WAIT_TIMEOUT_MS);

		switch (nfds) {

		case -1:
			// throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));
			log.warn("epoll_wait: " + std::string(strerror(errno)));
			break;
// DEBUG BEGIN
		case 0:
			log.debug("Timeout: no events");
			break;

		default:
			dumpEvents(nfds, _events);
			warnHighEventLoad(nfds, MAX_EPOLL_EVENTS);

			if (!_sockets.empty()) {
				std::map<int, ListeningSocket>::iterator it = _sockets.begin();
				while (it != _sockets.end()) {
					log.error("socket_" + i2a(it->first));
					++it;
				}
			}
			if (!_clients.empty()) {
				std::map<int, Client*>::iterator it = _clients.begin();
				while (it != _clients.end()) {
					log.error("client_" + i2a(it->first));
					++it;
				}
			}
			if (!_scripts.empty()) {
				std::map<int, int>::iterator it = _scripts.begin();
				while (it != _scripts.end()) {
					log.error("pipe_" + i2a(it->first));
					++it;
				}
			}

// DEBUG END
		}

		for (int n = 0; n < nfds; ++n) {
			int fd = _events[n].data.fd;
			epoll_event ev = _events[n];
			uint32_t events = ev.events;
			// bool tcp_peer_alive = true;

			std::map<int, ListeningSocket>::const_iterator listen_socket = _sockets.find(fd);
			if (listen_socket != _sockets.end() && events & EPOLLIN) {
				_acceptConnectRequest(listen_socket->first, listen_socket->second);
			}

			std::map<int, Client*>::iterator client_socket = _clients.find(fd);
			if (client_socket != _clients.end()) {

				if (events & EPOLLERR)
					_handleSocketError(client_socket);
				else if (events & EPOLLHUP)
					_cleanUpClient(client_socket);
				// else if (events & EPOLLRDHUP)
				// 	_cleanUpClient(client_socket);
				else if (events & (EPOLLIN | EPOLLRDHUP))
					_handleSocketReadEvent(client_socket);
				else if (events & EPOLLOUT)
					_handleSocketWriteEvent(client_socket);
			}

			std::map<int, int>::iterator script_pipe = _scripts.find(fd);
			if (script_pipe != _scripts.end()) {

				if (events & EPOLLERR)
					_handlePipeError(script_pipe);
				else if (events & EPOLLOUT)
					_handlePipeWriteEvent(script_pipe);
					// If pipe read end closed. write() will fail with EPIPE and raise
					// SIGPIPE which is currently ignored, see signal(SIGPIPE, SIG_IGN);
				else if (events & (EPOLLIN | EPOLLHUP))
					_handlePipeReadEvent(script_pipe);
				// else if (events & EPOLLHUP)
				// 	_handlePipeEOFEvent(script_pipe);
			}
		}

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

void Server::_acceptConnectRequest(int listen_fd, ListeningSocket socket) {

	log.info("New connection on socket fd_" + i2a(listen_fd));

	Client* c = new Client(socket.addr, socket.conf);

	int client_fd = accept(listen_fd, &c->getRemoteAddr(), &c->getRemoteAddrlen());
	if (client_fd == -1) {

		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			log.error("accept: " + std::string(strerror(errno)));
		}

		delete c;
		return;
	}

	_clients[client_fd] = c;
	// _reverse[c] = client_fd;

	if (!_setNonblockFlag(client_fd)) {
		_cleanUpClient(_clients.find(client_fd));
		return;
	}
	if (!_setPollInterest(client_fd)) {
		_cleanUpClient(_clients.find(client_fd));
		return;
	}

	log.info("client_" + i2a(client_fd) + ", endpoint "
				+ c->getRemoteAddress() + ":" + i2a(c->getRemotePort()));

	return;
}

void Server::_handleSocketError(std::map<int, Client*>::iterator it) {

	int error = 0;
	socklen_t len = sizeof(error);

	if (getsockopt(it->first, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
		log.error("getsockopt(SO_ERROR): " + std::string(strerror(errno)));
	} else if (error != 0) {
		log.error("socket error: " + std::string(strerror(error)));
	}

	// std::map<int, Client*>::iterator it = _clients.find(fd);

	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// }

	_cleanUpClient(it);
	return;

}

void Server::_handleSocketReadEvent(std::map<int, Client*>::iterator client_it) {

	// std::map<int, Client*>::iterator it = _clients.find(fd);

	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup:: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// 	return false;
	// }

	int client_fd = client_it->first;
	Client& client = *client_it->second;
	Buffer& incoming = client.getIncomingData();
	if (client.getState() == Client::REJECTED) {
		ssize_t bytes_received = client.queueIncomingData(client_fd);
		incoming.reset();
		if (bytes_received <= 0)
			_cleanUpClient(client_it);
		return;
	}
	if (client.getState() == Client::RECEIVING_BODY &&
		client.cgi_process != NULL && incoming.end == incoming.data.size()) {
		epoll_event event;
		event.events = EPOLLRDHUP;
		event.data.fd = client_fd;
		if (epoll_ctl(_epfd, EPOLL_CTL_MOD, client_fd, &event) == -1)
			_handleSocketError(client_it);
		return;
	}
	ssize_t bytes_received = client.queueIncomingData(client_fd);

	if (bytes_received < 0) {

		log.warn("recv: " + std::string(strerror(errno)));
		_cleanUpClient(client_it);
		return;

	} else if (bytes_received == 0) {
		log.info("Connection closed by client fd_" + i2a(client_fd));
		_cleanUpClient(client_it);
		return;

	} else {

		if (client.getState() != Client::RECEIVING_BODY || client.cgi_process == NULL)
			client.parseDataFromPeer();

		if (client.getState() == Client::RETRIEVING_SESSION) {
			session_manager.retrieveSession(client);
		}

		if (client.getState() == Client::DISPATCHING) {
			log.error("client fd_" + i2a(client_fd) + " state: DISPATCHING");
			dispatcher.handleRequest(client);
		}

		// TEST
		// register pipe fd and add it to _scripts
		if (client.cgi_process != NULL) {
			if (client.cgi_process->wantsRead()) {
				int std_out = client.cgi_process->stdoutFd();
				if (_scripts.find(std_out) == _scripts.end()) {
					_scripts[std_out] = client_fd;
					if (!_prepareScriptPipeEnd(std_out, true)) {
						std::map<int, int>::iterator it = _scripts.find(std_out);
						if (it != _scripts.end()) _handlePipeError(it);
						return;
					}
				}
			}

			if (client.getState() == Client::RECEIVING_BODY && client.cgi_process->wantsWrite()) {

				int std_in = client.cgi_process->stdinFd();
				if (_scripts.find(std_in) != _scripts.end()) {
					if (!_setWRONLYInterest(std_in, true)) {
						std::map<int, int>::iterator it = _scripts.find(std_in);
						if (it != _scripts.end()) _handlePipeError(it);
					}
					return;
				}
				// _pipes[std_in] = &client;
				_scripts[std_in] = client_fd;
				if (!_prepareScriptPipeEnd(std_in)) {
					// dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
					// 							client.getCurrentRequest().resolved.location,
					// 							client.getCurrentRequest().headers_only,
					// 							client.getCurrentResponse());
					// client.setState(Client::PENDING_RESPONSE);
					// log.debug("client_" + i2a(fd) + ": state set to PENDING_RESPONSE");
					// client.popRequest();
					// if (!_setWRONLYInterest(fd)) {
					// 	_cleanUpClient(it);
					// 	return false;
					// }
					// client.markForTermination();
					// return true;
					std::map<int, int>::iterator it = _scripts.find(std_in);
					if (it != _scripts.end()) {
						_handlePipeError(it);
					}
				}
				// _pipes[std_in] = &client;
				// Returning here as we have to wait for pipe readyness to start writing to std_in.
				// _handlePipeWriteEvent() takes it from here
				return;
			}

			if (client.getState() == Client::AWAITING_CGI_OUTPUT && client.cgi_process->wantsRead()) {

				client.popRequest();
				client.pushRequest();

				log.error("DING!");
				int std_out = client.cgi_process->stdoutFd();
				if (_scripts.find(std_out) != _scripts.end())
					return;
				// _pipes[std_out] = &client;
				_scripts[std_out] = client_fd;
				if (!_prepareScriptPipeEnd(std_out, true)) {
					// dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
					// 							  client.getCurrentRequest().resolved.location,
					// 							  client.getCurrentRequest().headers_only,
					// 							  client.getCurrentResponse());
					// client.setState(Client::PENDING_RESPONSE);
					// log.debug("client_" + i2a(fd) + ": state set to PENDING_RESPONSE");
					// client.popRequest();
					// if (!_setWRONLYInterest(fd)) {
					// 	_cleanUpClient(it);
					// 	return false;
					// }
					// client.markForTermination();
					// return true;
					std::map<int, int>::iterator it = _scripts.find(std_out);
					if (it != _scripts.end()) {
						_handlePipeError(it);
					}
				}
				// _pipes[std_out] = &client;
				// _setWRONLYInterest(fd);
				return;
			}
		}

		if (client.getState() == Client::RECEIVING_BODY) {
			log.error("client fd_" + i2a(client_fd) + " state: RECEIVING_BODY");
			client.parseDataFromPeer();
		}

		if (client.getState() == Client::PREPARING_RESPONSE) {
			log.error("client fd_" + i2a(client_fd) + " state: PREPARING_RESPONSE");
			dispatcher.handleRequest(client);
		}

		if (client.getState() == Client::PENDING_RESPONSE) {
			log.error("client fd_" + i2a(client_fd) + " state: PENDING_RESPONSE");
			client.popRequest();
			client.pushRequest();

			if (client.blockedFromReceiving()) {
				if (!_setWRONLYInterest(client_fd)) {
					_cleanUpClient(client_it);
				}
			} else {
				if (!_setRDWRInterest(client_fd)) {
					_cleanUpClient(client_it);
				}
			}
		}
		return;
	}
}

void Server::_handleSocketWriteEvent(std::map<int, Client*>::iterator it) {

	// std::map<int, Client*>::iterator it = _clients.find(fd);
 //
	// if (it == _clients.end() || it->second == NULL) {
	// 	// throw std::runtime_error("client lookup: " + std::string(NFIND_CLIENT));
	// 	log.warn("client lookup:: " + std::string(NFIND_CLIENT));
	// 	return;
	// }

	int fd = it->first;
	Client& client = *it->second;

	if (client.getState() == Client::AWAITING_CGI_OUTPUT) return;

	if (client.getState() == Client::CONCLUDED ||
		client.getState() == Client::REJECTED) {
		return;
	}

	if (client.getState() == Client::PENDING_RESPONSE) {
		client.queueOutgoingData();
		client.popResponse();
		client.pushResponse();
	}

	if (client.getState() == Client::SENDING_HEADERS ||
		client.getState() == Client::SENDING_BODY) {
		client.sendDataToTCPPeer(fd);
	}

	switch (client.getState()) {

	case Client::IDLE:
		if (!_setRDONLYInterest(fd)) {
			_cleanUpClient(it);
		}
		break;
	case Client::ERROR:
		_cleanUpClient(it);
		break;
	case Client::REJECTED:
		if (shutdown(fd, SHUT_WR) == -1)
			log.warn("shutdown: client_" + i2a(fd) + ": " + std::string(strerror(errno)));
		if (!_dropWriteInterest(fd)) {
			_cleanUpClient(it);
		}
		break;
	case Client::CONCLUDED:
		_cleanUpClient(it);
	default:
		break;

	}

	return;
}

void Server::_handlePipeError(std::map<int, int>::iterator script_it) {

	// int client_fd = -1;
	// std::map<Client*, int>::const_iterator ti = _reverse.find(script_it->second);
	// if (ti != _reverse.end()) {
	// 	client_fd = ti->second;
	// }

	Client* client = NULL;
	int client_fd = script_it->second;
	std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	if (client_it != _clients.end()) {
		client = client_it->second;
	}
	if (client == NULL) return;

	_cleanUpScriptPipeEnd(script_it);
	client->cgi_process->forceKill();
	dispatcher.buildErrorResponse(INTERNAL_SERVER_ERROR,
								  client->getCurrentRequest().resolved.location,
								  client->getCurrentRequest().headers_only,
								  client->getCurrentResponse());
	client->setState(Client::PENDING_RESPONSE);
	log.debug("client_" + i2a(client_fd) + ": state set to PENDING_RESPONSE");
	client->popRequest();
	client->pushRequest();
	if (!_setWRONLYInterest(client_fd)) {
		std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
		if (client_it != _clients.end()) {
			_cleanUpClient(client_it);
			return;
		}
	}
	client->markForTermination();

	return;
}

void Server::_handlePipeWriteEvent(std::map<int, int>::iterator script_it) {

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

	Client* client = NULL;
	int client_fd = script_it->second;
	std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	if (client_it != _clients.end()) {
		client = client_it->second;
	}
	if (client == NULL) return;

	client->parseDataFromPeer();
	Buffer& incoming = client->getIncomingData();
	if (client->getState() == Client::RECEIVING_BODY &&
		!_setRDONLYInterest(client_fd)) {
		_handleSocketError(client_it);
		return;
	}
	if (client->getState() == Client::RECEIVING_BODY && incoming.range() == 0) {
		epoll_event event;
		event.events = 0;
		event.data.fd = script_it->first;
		if (epoll_ctl(_epfd, EPOLL_CTL_MOD, script_it->first, &event) == -1)
			_handlePipeError(script_it);
		return;
	}

	if (client->getState() == Client::PREPARING_RESPONSE) {
		int std_in = script_it->first;
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, std_in, NULL) == -1)
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		_scripts.erase(script_it);
		client->cgi_process->closeStdin();
		// TODO decide:
		// calling dispatcher wouldn't be needed if _state
		// was set to AWAITING_CGI_OUTPUT at end of
		// parseDataFromPeer()
		dispatcher.handleRequest(*client);
	}

	if (client->getState() == Client::AWAITING_CGI_OUTPUT && client->cgi_process->wantsRead()) {

		client->popRequest();
		client->pushRequest();

		log.error("DONG!");
		int std_out = client->cgi_process->stdoutFd();
		if (_scripts.find(std_out) != _scripts.end())
			return;
		// _pipes[std_out] = client;
		_scripts[std_out] = client_fd;
		if (!_prepareScriptPipeEnd(std_out, true)) {
			// if (_epfd != -1) {
			// 	log.debug("Removing fd " + i2a(it->first) + " (script pipe end) from epoll instance");
			// 	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			// 		log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
			// 	}
			// }
			// if (it->first != -1) {
			// 	log.debug("Closing fd " + i2a(it->first) + " (script pipe end)");
			// 	if (close(it->first) == -1) {
			// 		log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
			// 	}
			// }
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
			std::map<int, int>::iterator it = _scripts.find(std_out);
			if (it != _scripts.end()) {
				_handlePipeError(it);
			}
		}
		// _pipes[std_out] = &client;
		// _setWRONLYInterest(client_fd);
	}

	return;
}

void Server::_handlePipeReadEvent(std::map<int, int>::iterator script_it) {

	// int fd = script_it->first;
	// int client_fd = -1;
	// std::map<Client*, int>::const_iterator ti = _reverse.find(script_it->second);
	// if (ti != _reverse.end()) {
	// 	client_fd = ti->second;
	// }
	// Client& client = *script_it->second;

	Client* client = NULL;
	int std_out = script_it->first;
	int client_fd = script_it->second;
	std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
	if (client_it != _clients.end()) {
		client = client_it->second;
	}
	if (client == NULL) return;

	// if (client.getState() != Client::AWAITING_CGI_OUTPUT) {
	// 	return;
	// }

	// if (!client.cgi_process->wantsRead()) {
	// 	client.cgi_process->buildResponse(client.getCurrentResponse(),
	// 									  client.getCurrentRequest().headers_only);
	// 	client.setState(Client::PENDING_RESPONSE);
	// }

	ssize_t bytes_read = client->cgi_process->queueIncomingData(std_out);

	if (bytes_read < 0) {

		log.warn("read: " + std::string(strerror(errno)));
		// _cleanUpScriptPipeEnd(it);
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
		_handlePipeError(script_it);

	} else if (bytes_read == 0) {

		log.info("Script delivered full response via fd_" + i2a(std_out));
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, std_out, NULL) == -1)
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		_scripts.erase(script_it);
		client->cgi_process->closeStdout();
		client->cgi_process->buildResponse(client->getCurrentResponse(),
										   client->getCurrentRequest().headers_only);
		client->cgi_process->tryReap();
		client->setState(Client::PENDING_RESPONSE);
		if (!_setWRONLYInterest(client_fd)) {
			std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
			if (client_it != _clients.end()) {
				_cleanUpClient(client_it);
				return;
			}
		}
		return;

	} else {

		// log.error("some bytes read from pipe");
		client->updateTimeStamp();
		// TEST have CGIProcess consume the data in the buffer
		try {
			// log.error("consuming bytes from pipe");
			client->cgi_process->consumeAvailableOutput();
		} catch (std::exception& e) {
			log.warn("read: " + std::string(e.what()));
			// _cleanUpScriptPipeEnd(it);
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
			_handlePipeError(script_it);
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
// 	_cleanUpScriptPipeEnd(it);
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

	std::map<int, Client*>::iterator immediate;
	std::map<int, Client*>::iterator it = _clients.begin();
	while (it != _clients.end()) {
		immediate = it;
		++it;

		if (immediate->second->isTimedOut(now)) {

			int fd = immediate->first;
			Client& client = *immediate->second;
// DEBUG BEGIN
			log.debug("client_" + i2a(fd)
			+ " idle time: " + i2a(client.getIdleTime()) + "s");
// DEBUG END
			log.warn("client_" + i2a(fd) + " timed out");

			if (client.getState() == Client::RECEIVING_HEADERS) {
				dispatcher.buildErrorResponse(REQUEST_TIMEOUT,
											client.getCurrentRequest().resolved.location,
											client.getCurrentRequest().headers_only,
											client.getCurrentResponse());
				client.setState(Client::PENDING_RESPONSE);
				log.debug("client_" + i2a(fd) + ": state set to PENDING_RESPONSE");
				client.popRequest();
				client.pushRequest();
				if (_setWRONLYInterest(fd)) {
					client.markForTermination();
					return;
				}
			}
			_cleanUpClient(immediate);
// DEBUG BEGIN
			if (_clients.empty()) {
				log.info("All clients disconnected");
			}
// DEBUG END
		}
	}

}

void Server::_cleanUpAllRessources(void) {

	if (!_scripts.empty()) {

		std::map<int, int>::iterator immediate;
		std::map<int, int>::iterator it = _scripts.begin();

		while (it != _scripts.end()) {
			immediate = it;
			++it;
			log.error("pipe_" + i2a(immediate->first));
			_cleanUpScriptPipeEnd(immediate);
		}
	}
	_scripts.clear();

	if (!_clients.empty()) {

		std::map<int, Client*>::iterator immediate;
		std::map<int, Client*>::iterator it = _clients.begin();

		while (it != _clients.end()) {
			immediate = it;
			++it;
			log.error("client_" + i2a(immediate->first));
			_cleanUpClient(immediate);
		}

	}
	_clients.clear();

	if (!_sockets.empty()) {

		std::map<int, ListeningSocket>::iterator immediate;
		std::map<int, ListeningSocket>::iterator it = _sockets.begin();

		while (it != _sockets.end()) {
			immediate = it;
			++it;
			log.error("socket_" + i2a(immediate->first));
			_cleanUpSocket(immediate);
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

	// _addr.clear();
	return;
}

void Server::_cleanUpScriptPipeEnd(std::map<int, int>::iterator it) {

	if (_epfd != -1) {
		log.debug("Removing fd " + i2a(it->first) + " (script pipe end) from epoll instance");
		if (epoll_ctl(_epfd, EPOLL_CTL_DEL, it->first, NULL) == -1) {
			log.warn("Error during cleanup: epoll_ctl: " + std::string(strerror(errno)));
		}
	}

	if (it->first != -1) {
		log.debug("Closing fd " + i2a(it->first) + " (script pipe end)");
		if (close(it->first) == -1) {
			log.warn("Error during cleanup: close: " + std::string(strerror(errno)));
		}
	}

	log.debug("Erasing container entry for above script pipe end");
	_scripts.erase(it);
	return;
}

void Server::_cleanUpClient(std::map<int, Client*>::iterator it) {

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

	if (it->second->cgi_process != NULL) {
		std::map<int, int>::iterator in_it = _scripts.find(it->second->cgi_process->stdinFd());
		if (in_it != _scripts.end()) {
			_cleanUpScriptPipeEnd(in_it);
		}
		std::map<int, int>::iterator out_it = _scripts.find(it->second->cgi_process->stdoutFd());
		if (out_it != _scripts.end()) {
			_cleanUpScriptPipeEnd(out_it);
		}
	}

	if (it->second != NULL) {
		delete it->second;
		it->second = NULL;
	}

	log.debug("Erasing container entry for above client");
	_clients.erase(it);
	return;

}

void Server::_cleanUpSocket(std::map<int, ListeningSocket>::iterator it) {

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
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

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
	// if (_epfd != -1 || !_sockfd.empty() || !_clients.empty()) {
	if (_epfd != -1 || !_sockets.empty() || !_clients.empty()) {
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
