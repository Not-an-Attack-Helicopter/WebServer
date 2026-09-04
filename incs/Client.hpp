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

#include "Config.hpp"
#include "Buffer.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <netinet/in.h>
// #include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <ctime>
#include <cstring>
#include <cstddef>

class Client {

public:

	Client(const sockaddr_in socket, const Config::Socket* config);
	~Client(void);

	enum State {
		IDLE,
		RECEIVING_HEADERS,
		DISPATCHING,
		RECEIVING_BODY,
		PREPARING_RESPONSE,
		AWAITING_CGI_OUTPUT,
		PENDING_RESPONSE,
		SENDING_HEADERS,
		SENDING_BODY,
		CONCLUDED,
		REJECTED,
		ERROR
   };

	struct Body {

		std::stringstream			temp;
		std::ifstream				file;
		std::size_t					size;
		Sink						sink;

		Body(void) : size(0), sink(NONE) {temp.clear();file.close();}

	};

	struct Response {

		std::stringstream			headers;
		Body						body;

		Response(void) {headers.clear();}

	};

// DEBUG BEGIN
	double							getIdleTime(void) const;
	unsigned short int				getRemotePort(void) const;
	const std::string				getRemoteAddress(void) const;
	const std::string				getBuffer(void) const;
// DEBUG END

	const State&					getState(void) const;

	// sockaddr*						getAddrPointer(void) const;
	sockaddr&						getRemoteAddr(void);

	// socklen_t*						getAddrlenPointer(void) const;
	socklen_t&						getRemoteAddrlen(void);

	// const Config*					getConfigPointer(void) const;
	const Config::Socket&			getConfig(void) const;

	HTTPRequest&					getCurrentRequest(void);
	HTTPRequest&					getRecentRequest(void);

	HTTPResponse&					getCurrentResponse(void);

	Buffer&							getIncomingData(void);

	void							setState(State state);

	bool							hasPendingResponse(void) const;
	bool							blockedFromReceiving() const;
	bool							markedForTermination() const;
	bool							isTimedOut(void) const;

	ssize_t							queueIncomingData(int fd, bool from_pipe = false);

	void							parseDataFromPeer(void);		// build request
	void							queueOutgoingData(void);		// build response
	void							sendDataToTCPPeer(int fd);		// send response to peer
	void							pushRequest(void);
	void							pushResponse(void);
	void							popRequest(void);
	void							popResponse(void);
	void							blockFromReceiving(void);
	void							markForTermination(void);
	void							reset(void);


private:

	Client(const Client& other);
	Client& operator = (const Client& other);

	static const time_t				IDLE_TIMEOUT_SECONDS		= 60;
	static const time_t				HEADER_TIMEOUT_SECONDS		= 10;
	static const time_t				DISPATCH_TIMEOUT_SECONDS	= 120;
	static const time_t				BODY_TIMEOUT_SECONDS		= 120;
	static const time_t				REJECTED_TIMEOUT_SECONDS	= 10;

	State							_state;

	bool							_blocked_from_receiving;
	bool							_marked_for_termination;

	sockaddr_in						_server_addr;
	sockaddr_storage				_remote_addr;
	socklen_t						_addrlen;

	const Config::Socket*			_config;

	std::deque<HTTPRequest*>		_request_queue;		// FIFO queue of requests to dispatch
	std::deque<HTTPResponse*>		_response_queue;	// FIFO queue of responses to send

	Buffer							_instream;
	Buffer							_outstream;
	Buffer							_pipestream;

	Response						_response;

	time_t							_last_event;

	std::size_t						_adjustBufferSize(std::size_t payload_size);

	void							_buffNflushErrorHandler(ssize_t bytes_sent, int fd);
	void							_stateTransitionHandler(int fd);
	void							_clearStream(std::stringstream& stream);
	void							_clearStream(std::ifstream& stream);

};

#endif
