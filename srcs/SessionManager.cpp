/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 13:55:05 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/07 13:55:06 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/SessionManager.hpp"
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
SessionManager& SessionManager::instance(void) {
	static SessionManager instance;
	return instance;
}

const Session* SessionManager::getSession(const std::string& session_id) {

	std::map<std::string, Session*>::const_iterator it = _sessions.find(session_id);
	if (it != _sessions.end()) {
		return it->second;
	} else {
		return NULL;
	}
}

static inline const std::string setUpCookieHeader(const std::string& session_id, const Session& session) {

	std::string cookie_header = "Session_ID=" + session_id;
	cookie_header += "; Max-Age=" + i2a(Session::LIFETIME);
	cookie_header += "; Path=/";
	const std::vector<std::string>& attributes = session.getAttributes();
	for (size_t i = 0; i < attributes.size(); ++i) {
		cookie_header += "; " + attributes[i];
	}

	return cookie_header;
}

void SessionManager::retrieveSession(Client& client) {

	const std::time_t now = std::time(NULL);

	HTTPResponse& response = client.getCurrentResponse();
	HTTPRequest& request = client.getCurrentRequest();

	std::string session_id = request.getSessionID();

	if (!session_id.empty()) {
		std::map<std::string, Session*>::iterator it = _sessions.find(session_id);
		if (it != _sessions.end()) {
			Session& session = *it->second;
			response.setHeader("Custom-Header1", "session found: " + session_id);
			if (session.getExpirationTime() > now) {
				session.updateTimeStamp(now);
				response.setHeader("Custom-Header2", "updated session end-of-life");
				// request.setSession(session);
				client.setState(Client::DISPATCHING);
				return;
			} else {
				_sessions.erase(it);
				response.setHeader("Custom-Header2", "session expired, assigning new session");
			}
		}
	}

	response.setHeader("Custom-Header1", "no session found");
	Session* session = new Session();
	// request.setSession(*session);
	do {
		session_id = randomHexString(SESSION_ID_BIT_WIDTH);
	} while (_sessions.find(session_id) != _sessions.end());
	_sessions[session_id] = session;
	request.setSessionID(session_id);
	const std::string cookie_header = setUpCookieHeader(session_id, *session);
	response.setHeader("Set-Cookie", cookie_header);
	client.setState(Client::DISPATCHING);
	return;
}

void SessionManager::setAttribute(const std::string& session_id, const std::string& attribute) {

	std::map<std::string, Session*>::iterator it = _sessions.find(session_id);
	if (it != _sessions.end()) {
		Session& session = *it->second;
		session.setAttribute(attribute);
	}
	return;
}

void SessionManager::_sweepExpiredSessions(const std::time_t now) {

	std::map<std::string, Session*>::iterator immediate;
	std::map<std::string, Session*>::iterator it = _sessions.begin();

	while (it != _sessions.end()) {

		immediate = it;
		++it;

		if (immediate->second->getExpirationTime() > now) {
			delete immediate->second;
			_sessions.erase(immediate);
		}
	}

	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
SessionManager::SessionManager(void) {
	log.debug("SessionManager Constructor called");
	return;
}

/*	@brief Destructor	*/
SessionManager::~SessionManager(void) {
	log.debug("SessionManager Destructor called");

	if (!_sessions.empty()) {
		std::map<std::string, Session*>::iterator immediate;
		std::map<std::string, Session*>::iterator it = _sessions.begin();
		while (it != _sessions.end()) {
			immediate = it;
			++it;
			delete immediate->second;
			_sessions.erase(immediate);
		}
		_sessions.clear();
	}

	return;
}

/*	@brief Copy Constructor	*/
SessionManager::SessionManager(const SessionManager& other) {
	*this = other;
	log.debug("SessionManager Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
SessionManager& SessionManager::operator = (const SessionManager& other) {
	if (this != &other) {
		log.debug("SessionManager Copy Assignment Operator called");
	}
	return *this;
}
