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

#include "Config.hpp"
#include <cstring>
#include <netinet/in.h>
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

enum IPCType {

	// UNINITIALIZED,
	LISTEN_SOCKET,
	CLIENT_SOCKET,
	SCRIPT_STD_IO
};

// Universal struct for all kinds of file descriptors that
// will have interests registered with the e_poll instance
struct IPC {

	IPCType									type;
	int										fd;
	IPC*									script_in;
	IPC*									script_out;
	IPC*									client_ipc;
	Client*									peer;
	sockaddr_in								addr;
	const Config::Socket*					conf;

	IPC(IPCType type)
	:	type(type),
		fd(0),
		script_in(NULL),
		script_out(NULL),
		client_ipc(NULL),
		peer(NULL),
		conf(NULL) {
		std::memset(&addr, 0, sizeof(addr));
	}
};

// struct ListeningSocket {
// 	sockaddr_in								addr;
// 	const Config::Socket*					conf;
//
// 	ListeningSocket(void) : conf(NULL) {
// 		std::memset(&addr, 0, sizeof(addr));
// 	}
// };

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
	bool									_setRDWRInterest(int fd, void* ptr);
	bool									_disableSocketIO(int fd, void* ptr);
	bool									_setPollInterest(int fd, void* ptr, bool is_pipe = false);
	bool									_setRDONLYInterest(int fd, void* ptr, bool is_pipe = false);
	bool									_setWRONLYInterest(int fd, void* ptr, bool is_pipe = false);
	bool									_prepareScriptPipeEnd(int fd, void* ptr, bool is_write_end = false);

	void									_acceptConnectRequest(int listen_fd, const sockaddr_in& addr,
																  const Config::Socket* conf);

	void									_handleSocketError(IPC* client_ipc);
	void									_handleSocketReadEvent(IPC* client_ipc);
	void									_handleSocketWriteEvent(IPC* client_ipc);
	void									_handlePipeError(IPC* script_ipc);
	void									_handlePipeWriteEvent(IPC* script_ipc);
	void									_handlePipeReadEvent(IPC* script_ipc);
	// void									_handlePipeEOFEvent(std::map<int, Client*>::iterator it);

	void									_reapStaleClients(const std::time_t now);

	void									_cleanUpAllRessources(void);
	void									_cleanUpPipeEnd(IPC* script_ipc);
	void									_cleanUpClient(IPC* client_ipc);
	void									_cleanUpSocket(IPC* socket_ipc);

	static const unsigned short				MAX_EPOLL_EVENTS = 512; // 64 - 512
	static const unsigned short				EPOLL_WAIT_TIMEOUT_MS = 293; // 100 - 5000 what about 293?
	static const unsigned short				STALE_CLIENT_REAP_INTERVAL = 2;
	static const unsigned short				EXPIRED_SESSIONS_SWEEP_INTERVAL = 307;

	int										_epfd;

	std::vector<IPC*>						_ipcs;

	// std::map<int, ListeningSocket>			_sockets;
	// std::map<int, Client*>					_clients;
	// std::map<int, int>						_scripts;

	epoll_event								_events[MAX_EPOLL_EVENTS];

	std::time_t								_last_sweep;
	std::time_t								_last_reap;

};

#endif
