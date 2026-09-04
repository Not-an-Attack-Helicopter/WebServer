/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPCookie.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 20:47:39 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/04 20:47:40 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_COOKIE_HPP
#define HTTP_COOKIE_HPP

#include "HTTPRequest.hpp"
#include <string>

namespace HTTPCookie {

	bool extractCookies(const std::string& cookie_header, HTTPRequest& request);

}

#endif
