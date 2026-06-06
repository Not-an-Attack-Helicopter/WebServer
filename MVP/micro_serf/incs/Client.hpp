/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 12:43:59 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/05 12:44:00 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
# define CLIENT_H

# include <string>
# include <sys/socket.h>

class Client {

	public:
		Client(void);
		~Client(void);
		Client(const Client& other);
		Client& operator = (const Client& other);

		const std::string&			getIncomingData(void) const;
		const std::string&			getOutgoingData(void) const;

		void						queueIncomingData(char buffer[1024]);
		void						queueOutgoingData(const std::string& message);

		bool						hasPendingData(void) const;

		int							flushPendingData(int fd);

		sockaddr*					getAddrPointer(void) const;

		socklen_t*					getAddrlenPointer(void) const;


	private:

		sockaddr_storage			_addr;

		socklen_t					_addrlen;


		std::string					_incomingData;
		std::string					_outgoingData;
};

#endif

// static int					count;
// const int					_id;
// int							_sockfd;

// sockaddr*					getSockAddr(void) const;
// int							getFildes(void) const;
// void						setSockAddr(void);
// void						setFildes(void);

// const std::string&			getReadBuffer(void) const;
// const std::string&			getWriteBuffer(void) const;

// void						setReadBuffer(void);
// void						setWriteBuffer(const std::string& data);
