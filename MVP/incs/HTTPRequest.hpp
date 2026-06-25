#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

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
		ParseState				getState() const;

		const std::string&		getMethod() const;
		const std::string&		getPath() const;
		const std::string&		getQuery() const;
		const std::string&		getVersion() const;
		bool					hasHeader(const std::string& key) const;
		const std::string&		getHeader(const std::string& key) const;

		size_t					getContentLength() const;
		const std::string&		getBody() const;

		void					reset();

		int						parse(const std::string& raw);

	private:
		// For "\n\n" on Unix
		static const size_t 					UNIX_LINE_ENDING_SIZE = 2;
		// For "\r\n\r\n" on Windows
		static const size_t 					WINDOWS_LINE_ENDING_SIZE = 4;

		ParseState								_state;

		bool									_is_unix_style;

		std::map<std::string, std::string>		_headers;

		std::string								_method;
		std::string								_path;
		std::string								_query;
		std::string								_version;
		std::string								_body;

		size_t									_content_length;

		size_t					_findHeaderEnd(const std::string& raw);
		bool					_parseHeaders(const std::string& raw, size_t header_end_pos);
		bool					_parseRequestLine(const std::string& line);
		bool					_parseHeaderLine(const std::string& line);
		bool					_parseBody(const std::string& raw, size_t header_end_pos);

};

#endif

// const std::map<std::string, std::string>&	getHeaders() const;
// const std::string&							getURI() const;
// bool										isComplete() const;
// bool										_complete; // Serves no purpose!
// std::string									_uri; // Never used!
// Feed raw bytes; returns true when a complete request has been parsed
// bool										parse(const std::string& raw);
// bool										_parseRequestLine(const std::string& line);
// bool										_parseHeaderLine(const std::string& line);
// bool										_parseBody(const std::string& raw, size_t header_end_pos);
