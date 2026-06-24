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

#pragma once

#include <sys/socket.h>
#include <string>
#include <ctime>
// #include <cstddef>
#include "HTTPRequest.hpp"

// DEBUG
enum AdminCommand {
	STOP = 2000
};
// DEBUG

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

		ssize_t						queueIncomingData(int fd);
		void						parseIncomingData(void);

		void						queueOutgoingData(const std::string& message);
		bool						hasPendingData(void) const;
		ssize_t						flushPendingData(int fd);

		bool						isTimedOut(void) const;
// DEBUG
		double						getIdleTime(void) const;
// DEBUG
		void						reset(void);



	private:
		Client(const Client& other);
		Client& operator = (const Client& other);

		static const time_t			CONNECTION_IDLE_TIMEOUT_SECONDS = 420;

		sockaddr_storage			_addr;

		socklen_t					_addrlen;

		char						_buffer[1024];

		std::string					_incomingData;
		std::string					_outgoingData;

		HTTPRequest					_request;
		// HTTPReponse					_reponse;

		time_t						_lastEvent;
};
