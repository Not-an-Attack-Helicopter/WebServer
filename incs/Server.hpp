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

# define INVALID_ADDR "No valid address string was provided for the specified \
address family."
# define NFIND_CLIENT "Client not found."

struct FD {
	enum Type {
		PIPE,
		SOCKET
	};
	int			fd;
	Type		type;
};

class Server {

public:
	static Server&							instance(void);

	bool									setNonblockFlag(FD fd);
	bool									setRDWRInterest(int fd);
	bool									dropWriteInterest(int fd);
	bool									setPollInterest(FD fd);
	bool									setRDONLYInterest(FD fd);
	bool									setWRONLYInterest(FD fd);

	void									prepareEPollInstance(void);
	void									prepareListeningPort(const Config::Socket& config);
	void									handleEvents(void);
	void									acceptConnectRequest(int fd, const Config::Socket* config);
	void									handleSocketError(int fd);
	void									handleHangup(int fd);
	void									handleRemoteHangup(int fd);

	bool									handleReadEvent(int fd);

	void									handleWriteEvent(int fd);
	void									cleanUpAllRessources(void);
	void									cleanUpClient(std::map<int, Client*>::iterator it);
	void									cleanUpSocket(std::map<int, const Config::Socket*>::iterator it);

private:
	Server(void);
	~Server(void);
	Server(const Server& other);
	Server& operator = (const Server& other);

	static const int						MAX_EPOLL_EVENTS = 64; // 64 - 512
	static const int						EPOLL_WAIT_TIMEOUT_MS = 5000; // 100 – 5000

// DEBUG BEGIN
	bool									_stop;
// DEBUG END

	int										_epfd;

	std::vector<sockaddr_in>				_addr;

	epoll_event								_events[MAX_EPOLL_EVENTS];

	std::map<int, const Config::Socket*>	_sockets;
	std::map<int, Client*>					_clients;
	std::vector<int>						_pipes;

};

#endif
