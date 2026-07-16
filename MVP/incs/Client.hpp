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
// #include <vector>
#include <deque>
#include <ctime>
// #include <cstddef>
// #include "types.hpp"
#include "Config.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
// #include "Dispatcher.hpp"

// DEBUG BEGIN
enum AdminCommand {
	STOP = 2000
};
// DEBUG END

// class HTTPResponse;

class Client {

	public:
		Client(const Config* config);
		~Client(void);

// DEBUG BEGIN
		unsigned short int			getHostPort(void) const;
		const std::string			getHostAddress(void) const;
		const std::string			getBuffer(void) const;
		const std::string&			getIncomingData(void) const;
		// const std::string&			getOutgoingData(void) const;
// DEBUG END
		// sockaddr*					getAddrPointer(void) const;
		sockaddr&					getAddr(void);

		// socklen_t*					getAddrlenPointer(void) const;
		socklen_t&					getAddrlen(void);

		// const Config*				getConfigPointer(void) const;
		const Config&				getConfig(void) const;
		const HTTPRequest&			getCurrentRequest(void) const;
		HTTPResponse&				getCurrentResponse(void);

		// bool						hasPendingRequest(void) const;
		bool						hasPendingResponse(void) const;
		bool						hasPendingData(void) const;

		ssize_t						queueIncomingData(int fd); // receive
		void						parseIncomingData(void); // build request

		void						pushRequest(void);
		void						pushResponse(void);
		void						popRequest(void);
		void						popResponse(void);
		void						handleRequest(void); // TEST
		void						buildCSSResponse(void); // TEST
		void						buildResponse(void); // TEST

// DEBUG BEGIN
		void						queueOutgoingData(const std::string& message);
// DEBUG END
		void						queueOutgoingData(void); // prepare
		ssize_t						flushPendingData(int fd); // send

		bool						isTimedOut(void) const;
// DEBUG BEGIN
		double						getIdleTime(void) const;
// DEBUG END
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

		// Dispatcher					_handler;
		// Dispatcher& _handler; // Shared stateless handler

		std::deque<HTTPRequest*>		_request_queue; // FIFO queue of parsed requests
		std::deque<HTTPResponse*>		_response_queue; // FIFO queue of responses to send

		time_t							_last_event;
};

// #include "templates.tpp"

#endif

// const std::string&			getOutgoingData(void) const;
