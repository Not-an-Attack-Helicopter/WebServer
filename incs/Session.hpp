/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/05 22:59:12 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/05 22:59:14 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <vector>
#include <ctime>

class Session {

public:

	Session(void);
	~Session(void);

	static const unsigned short						LIFETIME = 3600;

	const std::vector<std::string>&					getAttributes(void) const;

	std::time_t										getExpirationTime(void) const;

	void											updateTimeStamp(const std::time_t now);

	void											setAttribute(const std::string& attribute);

private:

	Session(const Session& other);
	Session& operator = (const Session& other);

	std::vector<std::string>						_attributes;

	std::time_t										_createdAt;
	std::time_t										_touchedAt;
	std::time_t										_expiresAt;

};

#endif
