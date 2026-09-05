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
#include <string>
#include <map>

class SessionManager {

private:
	std::map<std::string, Session> sessions;

};

#endif
