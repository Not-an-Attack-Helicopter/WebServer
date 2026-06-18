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

// DEBUG
		unsigned short				getHostPort(void) const;
		const std::string			getHostAddress(void) const;
		const std::string			getBuffer(void) const;
		const std::string&			getIncomingData(void) const;
		// const std::string&			getOutgoingData(void) const;
// DEBUG

		sockaddr*					getAddrPointer(void) const;

		socklen_t*					getAddrlenPointer(void) const;

		void						queueIncomingData(size_t len);
		void						queueOutgoingData(const std::string& message);

		bool						hasPendingData(void) const;

		ssize_t						fillPendingData(int fd);
		ssize_t						flushPendingData(int fd);

	private:
		Client(const Client& other);
		Client& operator = (const Client& other);

		sockaddr_storage			_addr;

		socklen_t					_addrlen;

		char						_buffer[1024];

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
