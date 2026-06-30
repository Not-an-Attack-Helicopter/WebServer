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
#include "HTTPRequest.hpp"
// #include "HTTPResponse.hpp"

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
// DEBUG

		sockaddr*					getAddrPointer(void) const;

		socklen_t*					getAddrlenPointer(void) const;

		ssize_t						queueIncomingData(int fd);
		// void						cleanIncomingData(void);
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

		static const time_t				CONNECTION_IDLE_TIMEOUT_SECONDS = 420;

		sockaddr_storage				_addr;

		socklen_t						_addrlen;

		char							_buffer[1024];

		std::string						_incoming_data;
		std::string						_outgoing_data;

		std::vector<HTTPRequest*>		_request_queue; // FIFO queue of parsed requests
		// std::vector<HTTPResponse*>		_response_queue; // FIFO queue of responses to send
		// RequestHandler& _handler; // Shared stateless handler
		// void processRequests() {
		// 	while (!_requestQueue.empty()) {
		// 		HTTPRequest request = _requestQueue.front();
		// 		_requestQueue.erase(_requestQueue.begin());
		// 		HTTPResponse response = _handler.handle(request);
		// 		_responseQueue.push_back(response);
		// 	}
		// }

		time_t							_last_event;
};

#endif

// const std::string&			getOutgoingData(void) const;
