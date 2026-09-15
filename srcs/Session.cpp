/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 16:29:03 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/07 16:29:04 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Session.hpp"
#include "../incs/Logger.hpp"

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
Session::Session(void)
  : 	_createdAt(std::time(NULL)),
		_touchedAt(_createdAt),
		_expiresAt(_touchedAt + LIFETIME) {
	log.debug("Session Constructor called");
	return;
}

/*	@brief Destructor	*/
Session::~Session(void) {
	log.debug("Session Destructor called");
	return;
}

const std::vector<std::string>& Session::getAttributes(void) const {
	return _attributes;
}

std::time_t Session::getExpirationTime(void) const {
	return _expiresAt;
}

void Session::updateTimeStamp(const std::time_t now) {
	_touchedAt = now;
	_expiresAt = _touchedAt + LIFETIME;
	return;
}

void Session::setAttribute(const std::string& attribute) {
	_attributes.push_back(attribute);
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
Session::Session(const Session& other) {
	*this = other;
	log.debug("Session Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
Session& Session::operator = (const Session& other) {
	if (this != &other) {
		log.debug("Session Copy Assignment Operator called");
	}
	return *this;
}
