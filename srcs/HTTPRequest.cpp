/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 17:39:21 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/25 17:39:23 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPRequest.hpp"
// #include "../incs/constexpr.hpp"
// #include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
// #include "../incs/Config.hpp"
// #include "../incs/utils.hpp"
// #include <sstream>
#include <cstring>
// #include <cctype>
#include <cstddef>
#include <cstdlib>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
HTTPRequest::HTTPRequest(void) {
	log.debug("HTTPRequest Constructor called");
	headers_only = false;
	is_multipart = false;
	body_chunked = false;
	created_file = false;
	_method = METHOD_COUNT;
	_path.clear();
	_query.clear();
	_version.clear();
	_headers.clear();
	return;
}

/*	@brief Destructor	*/
HTTPRequest::~HTTPRequest(void) {
	log.debug("HTTPRequest Destructor called");
	return;
}

const std::string* HTTPRequest::BodyPart::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it == _headers.end()) return NULL;
	return &it->second;
}

const std::string* HTTPRequest::RequestBody::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = _trailers.find(key);
	if (it == _trailers.end()) return NULL;
	return &it->second;
}

//getters
const Method& HTTPRequest::getMethod(void) const {
	return _method;
}

const std::string& HTTPRequest::getPath(void) const {
	return _path;
}

const std::string& HTTPRequest::getQuery(void) const {
	return _query;
}

const std::string& HTTPRequest::getVersion(void) const {
	return _version;
}

const std::string* HTTPRequest::getHeader(const std::string& key) const {

	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it == _headers.end()) return NULL;
	return &it->second;

}

// setters
void HTTPRequest::BodyPart::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
	return;
}

void HTTPRequest::RequestBody::setHeader(const std::string& key, const std::string& value) {
	_trailers[key] = value;
	return;
}

void HTTPRequest::setMethod(const Method& method) {
	_method = method;
}

void HTTPRequest::setPath(const std::string& path) {
	_path = path;
}

void HTTPRequest::setQuery(const std::string& query) {
	_query = query;
}

void HTTPRequest::setVersion(const std::string& version) {
	_version = version;
}

void HTTPRequest::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

bool HTTPRequest::extractContentLength(void) {

	const std::string* value = getHeader("content-length");
	if (value == NULL) return false; // empty value

	std::size_t size;
	char* endptr;
	size = static_cast<std::size_t>(strtoul((*value).c_str(), &endptr, 10));
	if (std::strcmp(endptr, (*value).c_str()) == 0 || *endptr != '\0')
		return false; // malformed content-length header

	body.size = size;
	return true;

}

void HTTPRequest::reset(void) {

	std::memset(static_cast<void*>(&parsing), 0, sizeof(parsing));
	resolved.method = METHOD_COUNT;
	resolved.domain = NULL;
	resolved.location = NULL;
	headers_only = false;
	created_file = false;
	_method = METHOD_COUNT;
	_path.clear();
	_query.clear();
	_version.clear();
	_headers.clear();

	return;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
HTTPRequest::HTTPRequest(const HTTPRequest& other)
	:	parsing(other.parsing),
		resolved(other.resolved),
		headers_only(other.headers_only),
		created_file(other.created_file),
		_method(other._method),
		_path(other._path),
		_query(other._query),
		_version(other._version),
		_headers(other._headers) {
	log.debug("HTTPRequest Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
HTTPRequest& HTTPRequest::operator = (const HTTPRequest& other) {
	if (this != &other) {
		parsing = other.parsing;
		resolved = other.resolved;
		headers_only = other.headers_only;
		created_file = other.created_file;
		_method = other._method;
		_path = other._path;
		_query = other._query;
		_version = other._version;
		_headers = other._headers;
	}
	log.debug("HTTPRequest Copy Assignment Operator called");
	return *this;
}
