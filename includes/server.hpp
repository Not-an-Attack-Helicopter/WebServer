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

# include <netinet/in.h>
# include <sys/epoll.h>
// # include <netdb.h>
# include <map>
# include <string>
# include <vector>
# include <exception>
# include "types.hpp"

class Client;

# define EINADDR "Did not provide a character string representing \
a valid network address in the specified address family."
# define ENOCLNT "Missing Client."

class Server {

	public:
		Server(void);
		~Server(void);

		void	setConfig(const Config& cfg);
		void	setNonblockFlag(int fd);
		void	setReadInterest(int fd);
		void	addWriteInterest(int fd);
		void	removeWriteInterest(int fd);
		void	prepareEPollInstance(void);
		void	prepareListeningPort(const std::string& address, unsigned short port,
				const ServerConfig* srv_cfg);
		void	handleIncomingEvents(void);
		void	acceptConnectRequest(int fd);
		void	handleReadEvent(int fd);
		void	handleWriteEvent(int fd);
		void	cleanUpAllRessources();
		void	cleanUpClient(std::map<int, Client*>::iterator it);

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

		class SetOptionException : public std::exception {
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

		class MissingClientException : public std::exception {
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
		Server(const Server& other);
		Server& operator = (const Server& other);

		static const int				MAXEVENTS = 64;

		bool							_stop;

		int								_epfd;

		std::vector<sockaddr_in>		_addr;
		std::vector<int>				_sockfd;

		epoll_event						_events[MAXEVENTS];

		std::map<int, Client*>			_clients;

		Config							_config;
		std::map<int, const ServerConfig*>	_listen_configs;

};

#endif