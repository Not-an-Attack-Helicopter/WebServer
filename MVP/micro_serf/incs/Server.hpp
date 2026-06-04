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

# include <exception>
// # include <string>
// # include <netdb.h>
# include <netinet/in.h>
// # include <sys/socket.h>
# include <map>
#include <sys/epoll.h>
# include "Client.hpp"

class Server {

	public:
		Server(void);
		~Server(void);
		Server(const Server& other);
		Server& operator = (const Server& other);

		void							setNonblockFlag(int fd);

		// void							bindSocket(void);
		// void							listenToSocket(void);
		void							prepareListeningPort(void);
		void							prepareEPollInstance(void);
		void							acceptConnectionRequest(int nfds);
		void							func1(void);

		class SocketException : public std::exception {
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

		class EPollCreateException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		class EPollControlException : public std::exception {
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

		class AcceptException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

	private:
		static const int				MAXEVENTS = 100;
		static const int				BACKLOG = 10;

		static const in_port_t			PORT = 4242;

		struct sockaddr_in				_sa;

		int								_status;
		int								_sockfd;
		int								_epfd;

		epoll_event						_ev;
		epoll_event						_events[MAXEVENTS];

		std::map<int, Client>			_clients;


};

#endif

		// struct sockaddr_storage			_ca;

		// socklen_t						_addrSize;

		// int								_clientfd;

		// static const char*				_message;
