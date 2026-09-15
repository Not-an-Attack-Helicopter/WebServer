/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/05 22:29:48 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/05 22:29:49 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "Session.hpp"
#include "Client.hpp"
#include <string>
#include <map>

#define session_manager SessionManager::instance()

class SessionManager {

public:

	static SessionManager&				instance(void);

	const Session*						getSession(const std::string& session_id);

	void								retrieveSession(Client& client);

	void								setAttribute(const std::string& session_id,
													 const std::string& attribute);

	void								_sweepExpiredSessions(const std::time_t now);

private:

	SessionManager(void);
	~SessionManager(void);
	SessionManager(const SessionManager& other);
	SessionManager& operator = (const SessionManager& other);

	static const unsigned short			SESSION_ID_BIT_WIDTH = 56;

	std::map<std::string, Session*>		_sessions;

};

#endif
