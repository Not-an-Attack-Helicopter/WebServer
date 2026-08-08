/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MultipartBody.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 08:25:10 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/08 08:25:12 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MULTIPARTBODY_HPP
#define MULTIPARTBODY_HPP

# include "../incs/Config.hpp"
#include <fstream>
#include <sstream>
#include <string>

class MultipartBody {

public:

	MultipartBody(const std::string& boundary);
	~MultipartBody(void);
	MultipartBody(const MultipartBody& other);
	MultipartBody& operator = (const MultipartBody& other);

	enum ParseState {
		READING_PART_HEADERS,
		READING_FILE_DATA,
		READING_BOUNDARY,
		COMPLETE,
		ERROR
	};

		struct Body {
		std::stringstream					temp;
		std::ofstream						file;
		size_t								size;
		Sink								sink;
	};

private:

	ParseState								_state;

	std::string _buffer;
	std::string _boundary;

	// Information about current part
	std::string _filename;
	std::string _content_type;

	bool _append(const char* data, std::size_t size);

};

#endif
