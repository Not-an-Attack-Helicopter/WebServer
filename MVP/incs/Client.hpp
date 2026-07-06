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

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <string>
#include <vector>
#include <ctime>
// #include <cstddef>
#include "types.hpp"
#include "HTTPRequest.hpp"
// #include "HTTPResponse.hpp"
// #include "RequestHandler.hpp"

// DEBUG
enum AdminCommand {
	STOP = 2000
};
// DEBUG

// class HTTPResponse;

class Client {

	public:
		Client(const Config* config);
		~Client(void);

// DEBUG
		unsigned short				getHostPort(void) const;
		const std::string			getHostAddress(void) const;
		const std::string			getBuffer(void) const;
		const std::string&			getIncomingData(void) const;
// DEBUG

		sockaddr*					getAddrPointer(void) const;

		socklen_t*					getAddrlenPointer(void) const;

		const Config*				getConfigPointer(void) const;

		ssize_t						queueIncomingData(int fd);
		// void						cleanIncomingData(void);
		void						parseIncomingData(void);

		// void						processNextRequest(void);
		// void						processRequests(void);
		// void						sendResponse(HTTPResponse* res);

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

		static const time_t				CONNECTION_IDLE_TIMEOUT_SECONDS = 420;

		sockaddr_storage				_addr;

		socklen_t						_addrlen;

		const Config*					_config;

		char							_buffer[1024];

		std::string						_incoming_data;
		std::string						_outgoing_data;

		// RequestHandler					_handler;
		// RequestHandler& _handler; // Shared stateless handler

		std::vector<HTTPRequest*>		_request_queue; // FIFO queue of parsed requests
		// std::vector<HTTPResponse*>		_response_queue; // FIFO queue of responses to send

		time_t							_last_event;
};

#endif

// const std::string&			getOutgoingData(void) const;
