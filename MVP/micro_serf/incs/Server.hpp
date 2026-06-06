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

#ifndef MICRO_SERVER_H
# define MICRO_SERVER_H

// # include <netdb.h>
# include <netinet/in.h>
// # include <sys/socket.h>
# include <sys/epoll.h>
// # include <string>
# include <exception>
# include <map>
# include "Client.hpp"

class Server {

	public:
		Server(void);
		~Server(void);
		Server(const Server& other);
		Server& operator = (const Server& other);

		void							setNonblockFlag(int fd);
		void							setReadInterest(int fd);
		void							addWriteInterest(int fd);
		void							removeWriteInterest(int fd);
		void							prepareListeningPort(void);
		void							prepareEPollInstance(void);
		void							handleIncomingEvents(void);
		void							acceptConnectRequest(void);
		void							handleReadEvent(epoll_event e);
		void							handleWriteEvent(epoll_event e);
		void							cleanUpAllRessources(void);

		class SocketException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class BindException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class ListenException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class CreateEPollException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class ModifyEPollException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class EventPollingException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class GetFlagsException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class SetFlagsException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class AcceptException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class ReadDataException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class FlushDataException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

	private:
		static const in_addr_t			SERVERADDRESS = INADDR_LOOPBACK;
		static const in_port_t			PORT = 4242;

		static const int				MAXEVENTS = 100;
		static const int				BACKLOG = 10;

		int								_sockfd;
		int								_epfd;

		sockaddr_in						_sa;

		epoll_event						_ev;
		epoll_event						_events[MAXEVENTS];

		std::map<int, Client>			_clients;

};

#endif

// void							createSocket(void);
// void							bindSocket(void);
// void							listenToSocket(void);

// class FcntlException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// int								_status;
// sockaddr_storage				_ca;
// socklen_t						_addrSize;
// int								_clientfd;
// static const char*				_message;
