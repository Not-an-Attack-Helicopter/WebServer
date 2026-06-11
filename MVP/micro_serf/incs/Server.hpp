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

#ifndef SERVER_H
# define SERVER_H

// # include <netdb.h>
# include <netinet/in.h>
// # include <sys/socket.h>
# include <sys/epoll.h>
// # include <string>
# include <exception>
# include <map>
# include "Client.hpp"

# define EINADDR	"Did not provide a character string representing a valid \
					network address in the specified address family.\n"

class Server {

	public:
		Server(void);
		~Server(void);
		Server(const Server& other);
		Server& operator = (const Server& other);

		void							setNonblockFlag(int fd);
		void							setReadInterest(int index, int fd);
		void							addWriteInterest(int index, int fd);
		void							removeWriteInterest(int index, int fd);
		// void							prepareListeningPort(void);
		void							prepareListeningPort(int index, const std::string& address, unsigned short port);
		void							prepareEPollInstance(int index);
		void							handleIncomingEvents(int index);
		void							acceptConnectRequest(int index);
		void							handleReadEvent(int index, int fd);
		void							handleWriteEvent(int index, int fd);
		void							cleanUpAllRessources(int index);
		void							cleanUpClient(int index, std::map<int, Client*>::iterator it);


		class AFNotSupportedException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class InvalidAddressException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

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

		static const int				MAXSOCKETS = 10;
		static const int				MAXEVENTS = 100;
		static const int				BACKLOG = 10;

		// change all these to std::map<int, whatever>! Fixed arrays are dumdum!
		bool							_shut[MAXSOCKETS];
		bool							_stop[MAXSOCKETS];
		// bool							_kill;
		int								_sockfd[MAXSOCKETS];
		int								_epfd[MAXSOCKETS];

		sockaddr_in						_sa[MAXSOCKETS];

		epoll_event						_events[MAXSOCKETS][MAXEVENTS];

		std::map<int, Client*>			_clients[MAXSOCKETS];

};

#endif

// void							createSocket(void);
// void							bindSocket(void);
// void							listenToSocket(void);

// class FcntlException : public std::exception {
// public:
// 	virtual const char*		what(void) const throw ();
// };

// static const in_addr_t			SERVERADDRESS = INADDR_LOOPBACK;
// static const in_port_t			PORT = 4242;
// int								_status;
// epoll_event						_ev;
// sockaddr_storage				_ca;
// socklen_t						_addrSize;
// int								_clientfd;
// static const char*				_message;
