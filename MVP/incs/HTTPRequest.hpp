#pragma once

#include <cstddef>
#include <string>
#include <map>

enum ParseState {
	PS_REQUEST_LINE,
	PS_READING_HEADERS,
	PS_READING_BODY,
	PS_COMPLETE,
	PS_ERROR
};

class HTTPRequest {

	public:
		HTTPRequest();
		HTTPRequest(const HTTPRequest& other);
		HTTPRequest& operator=(const HTTPRequest& other);
		~HTTPRequest();

		// Getters
		const std::string&							getMethod() const;
		const std::string&							getURI() const;
		const std::string&							getPath() const;
		const std::string&							getQuery() const;
		const std::string&							getVersion() const;
		const std::map<std::string, std::string>&	getHeaders() const;
		const std::string&							getHeader(const std::string& key) const;
		bool										hasHeader(const std::string& key) const;
		const std::string&							getBody() const;
		size_t										getContentLength() const;
		// bool										isComplete() const;
		ParseState									getState() const;

		// Feed raw bytes; returns true when a complete request has been parsed
		// bool										parse(const std::string& raw);
		int											parse(const std::string& raw);
		void										reset();

	private:
		std::string									_method;
		std::string									_uri;
		std::string									_path;
		std::string									_query;
		std::string									_version;
		std::map<std::string, std::string>			_headers;
		std::string									_body;
		size_t										_content_length;
		// bool										_complete; // Serves no purpose!
		ParseState									_state;

		bool										_parse_request_line(const std::string& line);
		bool										_parse_header_line(const std::string& line);
		bool										_parse_body(const std::string& raw, size_t header_end);

};
