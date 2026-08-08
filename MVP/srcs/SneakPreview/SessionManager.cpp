/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 21:03:42 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/28 21:03:52 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Session.hpp"

class SessionManager {

private:
	std::map<std::string, Session> sessions;

public:
	SessionManager() {
	}

	~SessionManager() {
	}

	std::string generateSessionId() {
		static unsigned long counter = 0;
		char buffer[64];
		srand(time(NULL) + counter++);
		sprintf(buffer, "SID_%ld_%d", time(NULL), rand());
		return std::string(buffer);
	}

	std::string createSession() {

		std::string sessionId = generateSessionId();
		Session newSession;
		newSession.setSessionId(sessionId);
		sessions[sessionId] = newSession;

		return sessionId;
	}

	bool getSessionData(const std::string& sessionId,
					std::string& value,
					const std::string& key) {
		// No locks needed — single-threaded epoll
		if (sessions.find(sessionId) == sessions.end()) {
			return false;
		}

		Session& session = sessions[sessionId];
		if (session.isExpired()) {
			sessions.erase(sessionId);
			return false;
		}

		// sd.lastAccessedAt = time(NULL);
		// value = sd.data[key];
		// return true;

		 session.updateLastAccessed();
		 bool result = session.getData(key, value);
		 return result;
	   }

	bool setSessionData(const std::string& sessionId,
	 const std::string& key,
	 const std::string& value) {

	  if (sessions.find(sessionId) == sessions.end()) {
	   return false;
	 }

	  Session& session = sessions[sessionId];

	  if (session.isExpired()) {
	   sessions.erase(sessionId);
	   return false;
	 }

	  session.updateLastAccessed();
	  session.setData(key, value);

	  return true;
	}

	void deleteSession(const std::string& sessionId) {
	 sessions.erase(sessionId);
   }
// ... other methods without locks ...
};
