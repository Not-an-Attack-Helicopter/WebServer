/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MicroServer.hpp                                    :+:      :+:    :+:   */
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
# include <sys/socket.h>

class MicroServer {

	public:
		MicroServer(void);
		~MicroServer(void);
		MicroServer(const MicroServer& other);
		MicroServer& operator = (const MicroServer& other);

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

		class AcceptException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

	private:
		struct sockaddr_in				_sa;
		struct sockaddr_storage			_client_addr;

		socklen_t						_addr_size;

		int								_socket_fd;
		int								_client_fd;
		static int						_status;

		static const in_port_t			PORT = 4242;
		static const int				BACKLOG = 10;

};

#endif
