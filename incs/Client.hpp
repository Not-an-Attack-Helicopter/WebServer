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
#include "CGIProcess.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <netinet/in.h>
// #include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <string>
// #include <deque>
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
		RETRIEVING_SESSION,
		RECEIVING_BODY,
		DISPATCHING,
		AWAITING_CGI_OUTPUT,
		PREPARING_RESPONSE,
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

	CGIProcess*						cgi_process; // owns the live CGI child while one is running (NULL otherwise)
	// std::deque<CGIProcess*>			process_queue;

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
	// HTTPRequest&					getRecentRequest(void);

	HTTPResponse&					getCurrentResponse(void);

	Buffer&							getIncomingData(void);

	void							setState(State state);

	bool							hasPendingResponse(void) const;
	bool							blockedFromReceiving() const;
	bool							markedForTermination() const;
	bool							isTimedOut(const std::time_t now) const;

	ssize_t							queueIncomingData(int fd);

	void							parseDataFromPeer(void);		// build request
	void							queueOutgoingData(void);		// build response
	void							sendDataToTCPPeer(int fd);		// send response to peer
	void							pushRequest(void);
	void							pushResponse(void);
	void							popProcess(void);
	void							popRequest(void);
	void							popResponse(void);
	void							blockFromReceiving(void);
	void							markForTermination(void);
	void							updateTimeStamp(void);
	void							reset(void);

private:

	Client(const Client& other);
	Client& operator = (const Client& other);

	// static const unsigned short		REQUEST_ID_BIT_WIDTH = 48;

	static const std::time_t		IDLE_TIMEOUT_SECONDS		= 60;
	static const std::time_t		HEADER_TIMEOUT_SECONDS		= 12;
	static const std::time_t		BODY_TIMEOUT_SECONDS		= 120;
	static const std::time_t		PROCESSING_TIMEOUT_SECONDS	= 420;
	static const std::time_t		REJECTED_TIMEOUT_SECONDS	= 10;

	State							_state;

	bool							_blocked_from_receiving;
	bool							_marked_for_termination;

	const Config::Socket*			_config;

	sockaddr_in						_server_addr;
	sockaddr_storage				_remote_addr;
	socklen_t						_addrlen;

	// std::deque<HTTPRequest*>		_request_queue;		// FIFO queue of requests to dispatch
	// std::deque<HTTPResponse*>		_response_queue;	// FIFO queue of responses to send

	HTTPRequest*					_request;
	HTTPResponse*					_response;

	Buffer							_instream;
	Buffer							_outstream;

	Response						_pending_response;

	std::time_t						_last_event;

	std::size_t						_adjustBufferSize(std::size_t payload_size);

	void							_stateTransitionHandler(int fd);
	void							_clearStream(std::istream& stream);

};

#endif
