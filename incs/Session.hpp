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
#include <ctime>
#include <map>

class Session {

private:

	std::string								_session_id;

	std::map<std::string, std::string>		_data;

	time_t									_createdAt;
	time_t 									_touchedAt;
	time_t 									_expiresAt;

};

#endif
