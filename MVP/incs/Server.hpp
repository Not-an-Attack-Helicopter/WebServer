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

#include "types.hpp"
#define server Server::instance()

#include <netinet/in.h>
#include <sys/epoll.h>
#include <netdb.h>
// #include <string>
#include <vector>
#include <map>
#include "Client.hpp"
// # include "types.hpp"

# define INVALID_ADDR "No valid address string was provided for the specified \
address family."
# define NFIND_CLIENT "Client not found."

class Server {

	public:
		static Server&					instance(void);

		void							setNonblockFlag(int fd);
		void							setReadInterest(int fd);
		void							addWriteInterest(int fd);
		void							removeWriteInterest(int fd);
		void							prepareEPollInstance(void);
		// void							prepareListeningPort(const std::string& address, unsigned short port);
		void							prepareListeningPort(const Config& config);
		void							handleIncomingEvents(void);
		void							acceptConnectRequest(int fd);
		bool							handleReadEvent(int fd);
		void							handleWriteEvent(int fd);
		void							cleanUpAllRessources();
		void							cleanUpClient(std::map<int, Client*>::iterator it);
		void							cleanUpSocket(std::map<int, const Config*>::iterator it);

	private:
		Server(void);
		~Server(void);
		Server(const Server& other);
		Server& operator = (const Server& other);

		static const int				MAX_EPOLL_EVENTS = 64; // 64 - 512
		static const int				EPOLL_WAIT_TIMEOUT_MS = 5000; // 100 – 5000

		bool							_stop;

		int								_epfd;

		std::vector<sockaddr_in>		_addr;

		epoll_event						_events[MAX_EPOLL_EVENTS];

		std::map<int, const Config*>	_sockets;
		std::map<int, Client*>			_clients;

// TEST
		void							forking_around(int socket_fd, int client_fd);
// TEST

};

#endif

// class CreateEPollException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class ModifyEPollException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class AFNotSupportedException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class InvalidAddressException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class SocketException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class SetSockOptionException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class BindException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class ListenException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class EventPollingException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class GetFileStatusException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class SetFileStatusException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class AcceptException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class MissingClientException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class ReadDataException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// class FlushDataException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// void							createSocket(void);
// void							bindSocket(void);
// void							listenToSocket(void);

// class FcntlException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// static const in_addr_t			SERVERADDRESS = INADDR_LOOPBACK;
// static const in_port_t			PORT = 4242;
// static const int				MAXSOCKETS = 10;
// static const int				BACKLOG = 10;
// bool							_kill;
// int								_status;
// sockaddr_in						_sa;
// sockaddr_storage				_ca;
// socklen_t						_addrSize;
// epoll_event						_ev;
// std::vector<int>				_sockfd;
// int								_clientfd;
// static const char*				_message;
