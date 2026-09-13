/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 06:03:28 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/04 06:03:31 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#define server Server::instance()

#include "Client.hpp"
// #include <netinet/in.h>
#include <sys/epoll.h>
// #include <netdb.h>
// #include <string>
// #include <vector>
// #include <map>

#define INVALID_ADDR "No valid address string was provided for the specified \
address family."
// #define NFIND_CLIENT "Client not found."
// #define NFIND_SCRIPT "CGI process not found."

struct ListeningSocket {
	sockaddr_in								addr;
	const Config::Socket*					conf;
};

class Server {

public:

	static Server&							instance(void);

	void									prepareEPollInstance(void);
	void									prepareListeningPort(const Config::Socket& config);
	void									handleEvents(void);

private:

	Server(void);
	~Server(void);
	Server(const Server& other);
	Server& operator = (const Server& other);

	bool									_setNonblockFlag(int fd);
	bool									_setRDWRInterest(int fd);
	bool									_dropWriteInterest(int fd);
	bool									_setPollInterest(int fd, bool is_pipe = false);
	bool									_setRDONLYInterest(int fd, bool is_pipe = false);
	bool									_setWRONLYInterest(int fd, bool is_pipe = false);
	bool									_prepareScriptPipeEnd(int fd, bool is_read_end = false);

	void									_acceptConnectRequest(int fd, ListeningSocket socket);

	void									_handleSocketError(std::map<int, Client*>::iterator it);
	void									_handleSocketReadEvent(std::map<int, Client*>::iterator it);
	void									_handlePipeWriteEvent(std::map<int, Client*>::iterator it);
	void									_handlePipeReadEvent(std::map<int, Client*>::iterator it);
	// void									_handlePipeEOFEvent(std::map<int, Client*>::iterator it);
	void									_handlePipeError(std::map<int, Client*>::iterator it);
	void									_handleSocketWriteEvent(std::map<int, Client*>::iterator it);

	void									_reapStaleClients(const std::time_t now);

	void									_cleanUpAllRessources(void);
	void									_cleanUpScriptPipeEnd(std::map<int, Client*>::iterator it);
	void									_cleanUpClient(std::map<int, Client*>::iterator it);
	void									_cleanUpSocket(std::map<int, ListeningSocket>::iterator it);

	static const unsigned short				MAX_EPOLL_EVENTS = 64; // 64 - 512
	static const unsigned short				EPOLL_WAIT_TIMEOUT_MS = 5000; // 100 - 5000 what about 293?
	static const unsigned short				STALE_CLIENT_REAP_INTERVAL = 2;
	static const unsigned short				EXPIRED_SESSIONS_SWEEP_INTERVAL = 307;

	int										_epfd;

	// std::vector<sockaddr_in>				_addr;

	std::map<int, ListeningSocket>			_sockets;
	std::map<int, Client*>					_clients;
	std::map<Client*, int>					_reverse;
	std::map<int, Client*>					_scripts;

	epoll_event								_events[MAX_EPOLL_EVENTS];

	std::time_t								_last_sweep;
	std::time_t								_last_reap;

};

#endif
