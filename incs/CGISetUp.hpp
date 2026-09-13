/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISetUp.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gpochon <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 08:48:07 by gpochon           #+#    #+#             */
/*   Updated: 2026/09/13 08:48:09 by gpochon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_SETUP_HPP
#define CGI_SETUP_HPP

# include "Client.hpp"

// Builds CGI environment, args array, and spawns the child,
// stores CGIProcess object in the client so stdin and stdout can be registered with epoll
StatusCode setUpCGI(Client& client);

#endif
