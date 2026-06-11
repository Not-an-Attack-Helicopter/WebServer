/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AddressInfo.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 21:10:19 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/02 21:10:21 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ADDRESS_INFO_H
# define ADDRESS_INFO_H

# include <exception>
# include <string>
# include <netdb.h>

class AddressInfo {

	public:
		~AddressInfo();

		void						gai(void);

		class GetAddressInfoException : public std::exception {
		public:
			virtual const char*		what(void) const throw ();
		};

		static AddressInfo&			getInstance(const std::string& hostname);

	private:
		static AddressInfo*			_instance;

		AddressInfo(const std::string& hostname);

		const std::string			_hostname;
		static int					_status;
		struct addrinfo*			_pai;
		struct addrinfo				_req;
		char						_buff[INET_ADDRSTRLEN];
		static const int			LEN = sizeof(_buff);
		char						_buff6[INET6_ADDRSTRLEN];
		static const int			LEN6 = sizeof(_buff6);

		// AddressInfo(const AddressInfo& other);
		// AddressInfo& operator = (const AddressInfo& other);

};

#endif
