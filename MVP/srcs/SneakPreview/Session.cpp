/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 20:25:54 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/28 20:25:55 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Session.hpp"

Session::Session(void)
	:	_createdAt(time(NULL)),
		_lastAccessedAt(time(NULL)),
		_expiryMinutes(30) {}

// Getters
std::string Session::getSessionId(void) const {
	return _sessionId;
}

int Session::getExpiryMinutes(void) const {
	return _expiryMinutes;
}

time_t Session::getCreatedAt(void) const {
	return _createdAt;
}

time_t Session::getLastAccessedAt(void) const {
	return _lastAccessedAt;
}

// Setters
void Session::setSessionId(const std::string& id) {
	_sessionId = id;
}

void Session::setExpiryMinutes(int minutes) {
	_expiryMinutes = minutes;
}

// Data management
bool Session::getData(const std::string& key, std::string& value) {
	if (_data.find(key) != _data.end()) {
	value = _data[key];
	return true;
}
	return false;
}

void Session::setData(const std::string& key, const std::string& value) {
	_data[key] = value;
}

void Session::removeData(const std::string& key) {
	_data.erase(key);
}

void Session::clearData(void) {
	_data.clear();
}

// Access tracking
void Session::updateLastAccessed() {
	_lastAccessedAt = time(NULL);
}

// Expiry check
bool Session::isExpired(void) const {
	return (time(NULL) - _lastAccessedAt) > (_expiryMinutes * 60);
}
