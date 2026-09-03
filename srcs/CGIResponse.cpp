#ifndef CGI_RESPONSE_HPP
#define CGI_RESPONSE_HPP

#include <string>
#include <cstddef>
#include <cctype>

/*
 * Assumed to already exist:
 *
 * enum StatusCode { ... };
 *
 * class HTTPResponse {
 * public:
 *     void setStatus(StatusCode code);
 *     void setStatus(StatusCode code, const std::string& reason);
 *     void setHeader(const std::string& key, const std::string& value);
 *     void setBody(const std::string& body,
 *                  Sink body_sink,
 *                  const std::string& content_type,
 *                  bool headers_only);
 *     void reset(void);
 * };
 *
 * void errorPage(const std::string& location,
 *                HTTPResponse& response,
 *                bool headers_only,
 *                StatusCode code);
 */

class CgiResponse
{
public:

    enum State
    {
        HEADERS,
        BODY,
        DONE,
        ERROR
    };

private:

    static const std::size_t MAX_HEADER_SIZE = 32 * 1024;

    State           _state;
    std::size_t     _header_size;

    std::string     _line;
    std::string     _body;

    HTTPResponse&   _response;

    const std::string& _error_location;
    bool            _headers_only;

    bool            _has_status;
    bool            _has_location;
    bool            _has_content_type;

    std::string     _content_type;

private:

    CgiResponse(const CgiResponse&);
    CgiResponse& operator=(const CgiResponse&);

    static bool _equalsIgnoreCase(const std::string& a,
                                  const std::string& b)
    {
        if (a.size() != b.size())
            return false;

        std::size_t i;

        for (i = 0; i < a.size(); ++i)
        {
            unsigned char ca =
                static_cast<unsigned char>(a[i]);
            unsigned char cb =
                static_cast<unsigned char>(b[i]);

            if (std::tolower(ca) != std::tolower(cb))
                return false;
        }

        return true;
    }

    static bool _isTokenChar(unsigned char c)
    {
        if (c <= 31 || c == 127)
            return false;

        switch (c)
        {
            case '(':
            case ')':
            case '<':
            case '>':
            case '@':
            case ',':
            case ';':
            case ':':
            case '\\':
            case '"':
            case '/':
            case '[':
            case ']':
            case '?':
            case '=':
            case '{':
            case '}':
            case ' ':
            case '\t':
                return false;
        }

        return true;
    }

    static bool _validFieldName(const std::string& name)
    {
        if (name.empty())
            return false;

        std::size_t i;

        for (i = 0; i < name.size(); ++i)
        {
            if (!_isTokenChar(
                    static_cast<unsigned char>(name[i])))
                return false;
        }

        return true;
    }

    static std::string _trimOWS(const std::string& value)
    {
        std::size_t first = 0;
        std::size_t last = value.size();

        while (first < last &&
               (value[first] == ' ' ||
                value[first] == '\t'))
            ++first;

        while (last > first &&
               (value[last - 1] == ' ' ||
                value[last - 1] == '\t'))
            --last;

        return value.substr(first, last - first);
    }

    static bool _parseStatus(const std::string& value,
                             StatusCode& code,
                             std::string& reason)
    {
        /*
         * CGI Status:
         *
         *     3DIGIT SP reason-phrase
         */

        if (value.size() < 5)
            return false;

        if (value[3] != ' ')
            return false;

        if (!std::isdigit(
                static_cast<unsigned char>(value[0])) ||
            !std::isdigit(
                static_cast<unsigned char>(value[1])) ||
            !std::isdigit(
                static_cast<unsigned char>(value[2])))
            return false;

        int number =
            (value[0] - '0') * 100 +
            (value[1] - '0') * 10 +
            (value[2] - '0');

        if (number < 100 || number > 999)
            return false;

        std::size_t i;

        for (i = 4; i < value.size(); ++i)
        {
            unsigned char c =
                static_cast<unsigned char>(value[i]);

            if (c < 32 || c == 127)
                return false;
        }

        /*
         * Your StatusCode enum only represents known HTTP
         * status codes. A CGI Status outside that set cannot
         * be represented by HTTPResponse::setStatus(StatusCode).
         *
         * The mapping below intentionally accepts only codes
         * represented by your enum.
         */
        switch (number)
        {
            case 100: code = CONTINUE; break;
            case 101: code = SWITCHING_PROTOCOLS; break;
            case 102: code = PROCESSING; break;
            case 103: code = EARLY_HINTS; break;

            case 200: code = OK; break;
            case 201: code = CREATED; break;
            case 202: code = ACCEPTED; break;
            case 203: code = NON_AUTHORITATIVE_INFORMATION; break;
            case 204: code = NO_CONTENT; break;
            case 205: code = RESET_CONTENT; break;
            case 206: code = PARTIAL_CONTENT; break;
            case 207: code = MULTI_STATUS; break;
            case 208: code = ALREADY_REPORTED; break;
            case 226: code = IM_USED; break;

            case 300: code = MULTIPLE_CHOICES; break;
            case 301: code = MOVED_PERMANENTLY; break;
            case 302: code = FOUND; break;
            case 303: code = SEE_OTHER; break;
            case 304: code = NOT_MODIFIED; break;
            case 305: code = USE_PROXY; break;
            case 307: code = TEMPORARY_REDIRECT; break;
            case 308: code = PERMANENT_REDIRECT; break;

            case 400: code = BAD_REQUEST; break;
            case 401: code = UNAUTHORIZED; break;
            case 402: code = PAYMENT_REQUIRED; break;
            case 403: code = FORBIDDEN; break;
            case 404: code = NOT_FOUND; break;
            case 405: code = METHOD_NOT_ALLOWED; break;
            case 406: code = NOT_ACCEPTABLE; break;
            case 407: code = PROXY_AUTHENTICATION_REQUIRED; break;
            case 408: code = REQUEST_TIMEOUT; break;
            case 409: code = CONFLICT; break;
            case 410: code = GONE; break;
            case 411: code = LENGTH_REQUIRED; break;
            case 412: code = PRECONDITION_FAILED; break;
            case 413: code = PAYLOAD_TOO_LARGE; break;
            case 414: code = URI_TOO_LONG; break;
            case 415: code = UNSUPPORTED_MEDIA_TYPE; break;
            case 416: code = RANGE_NOT_SATISFIABLE; break;
            case 417: code = EXPECTATION_FAILED; break;
            case 418: code = IM_A_TEAPOT; break;
            case 421: code = MISDIRECTED_REQUEST; break;
            case 422: code = UNPROCESSABLE_ENTITY; break;
            case 423: code = LOCKED; break;
            case 424: code = FAILED_DEPENDENCY; break;
            case 425: code = TOO_EARLY; break;
            case 426: code = UPGRADE_REQUIRED; break;
            case 428: code = PRECONDITION_REQUIRED; break;
            case 429: code = TOO_MANY_REQUESTS; break;
            case 431: code = REQUEST_HEADER_FIELDS_TOO_LARGE; break;
            case 451: code = UNAVAILABLE_FOR_LEGAL_REASONS; break;

            case 500: code = INTERNAL_SERVER_ERROR; break;
            case 501: code = NOT_IMPLEMENTED; break;
            case 502: code = BAD_GATEWAY; break;
            case 503: code = SERVICE_UNAVAILABLE; break;
            case 504: code = GATEWAY_TIMEOUT; break;
            case 505: code = HTTP_VERSION_NOT_SUPPORTED; break;
            case 506: code = VARIANT_ALSO_NEGOTIATES; break;
            case 507: code = INSUFFICIENT_STORAGE; break;
            case 508: code = LOOP_DETECTED; break;
            case 510: code = NOT_EXTENDED; break;
            case 511: code = NETWORK_AUTHENTICATION_REQUIRED; break;

            default:
                return false;
        }

        reason = value.substr(4);
        return true;
    }

    static bool _isForbiddenHeader(const std::string& name)
    {
        /*
         * The HTTP server controls message framing and connection
         * management. These are not copied from CGI.
         */
        return _equalsIgnoreCase(name, "Content-Length") ||
               _equalsIgnoreCase(name, "Transfer-Encoding") ||
               _equalsIgnoreCase(name, "Connection") ||
               _equalsIgnoreCase(name, "Keep-Alive") ||
               _equalsIgnoreCase(name, "Upgrade") ||
               _equalsIgnoreCase(name, "TE") ||
               _equalsIgnoreCase(name, "Trailer");
    }

    bool _fail(StatusCode code)
    {
        _state = ERROR;

        _response.reset();

        errorPage(_error_location,
                  _response,
                  _headers_only,
                  code);

        return false;
    }

    bool _parseHeaderLine(const std::string& line)
    {
        /*
         * RFC 3875 does not permit folded/continued CGI headers.
         */
        if (!line.empty() &&
            (line[0] == ' ' || line[0] == '\t'))
            return false;

        std::string::size_type colon = line.find(':');

        if (colon == std::string::npos)
            return false;

        std::string name = line.substr(0, colon);

        if (!_validFieldName(name))
            return false;

        std::string value;

        if (colon + 1 < line.size())
            value = _trimOWS(line.substr(colon + 1));

        /*
         * CGI/1.1 requires these CGI fields to occur at most once.
         */
        if (_equalsIgnoreCase(name, "Status"))
        {
            if (_has_status)
                return false;

            StatusCode code;
            std::string reason;

            if (!_parseStatus(value, code, reason))
                return false;

            _has_status = true;

            _response.setStatus(code, reason);
            return true;
        }

        if (_equalsIgnoreCase(name, "Content-Type"))
        {
            if (_has_content_type)
                return false;

            _has_content_type = true;
            _content_type = value;

            _response.setHeader(name, value);
            return true;
        }

        if (_equalsIgnoreCase(name, "Location"))
        {
            if (_has_location)
                return false;

            _has_location = true;

            _response.setHeader(name, value);
            return true;
        }

        /*
         * Server-generated framing.
         */
        if (_isForbiddenHeader(name))
            return true;

        /*
         * All other CGI response headers are passed through.
         */
        _response.setHeader(name, value);
        return true;
    }

    bool _finishHeaders()
    {
        /*
         * CGI response with Location and no Status:
         * client redirect => 302.
         */
        if (_has_location && !_has_status)
            _response.setStatus(FOUND);

        _state = BODY;
        return true;
    }

public:

    CgiResponse(HTTPResponse& response,
                const std::string& error_location,
                bool headers_only)
        : _state(HEADERS),
          _header_size(0),
          _response(response),
          _error_location(error_location),
          _headers_only(headers_only),
          _has_status(false),
          _has_location(false),
          _has_content_type(false)
    {
    }

    /*
     * Feed bytes read from cgi_stdout.
     *
     * Returns false if the CGI response is invalid.
     *
     * Once the CGI headers are complete, all remaining bytes
     * are body bytes.
     */
    bool feed(const char* data, std::size_t size)
    {
        if (_state == ERROR || _state == DONE)
            return false;

        std::size_t i = 0;

        while (i < size)
        {
            if (_state == BODY)
            {
                _body.append(data + i, size - i);
                return true;
            }

            char c = data[i++];

            ++_header_size;

            if (_header_size > MAX_HEADER_SIZE)
                return _fail(INTERNAL_SERVER_ERROR);

            if (c == '\n')
            {
                /*
                 * Accept LF and CRLF.
                 */
                if (!_line.empty() &&
                    _line[_line.size() - 1] == '\r')
                {
                    _line.erase(_line.size() - 1);
                }

                /*
                 * Empty line terminates CGI headers.
                 */
                if (_line.empty())
                {
                    _line.clear();

                    if (!_finishHeaders())
                        return false;
                }
                else
                {
                    if (!_parseHeaderLine(_line))
                        return _fail(INTERNAL_SERVER_ERROR);

                    _line.clear();
                }
            }
            else
            {
                _line += c;
            }
        }

        return true;
    }

    /*
     * Must be called when read(cgi_stdout) reports EOF.
     */
    bool finish()
    {
        if (_state == ERROR)
            return false;

        if (_state == HEADERS)
            return _fail(INTERNAL_SERVER_ERROR);

        if (_state == DONE)
            return true;

        /*
         * A CGI document response containing an entity body
         * requires Content-Type.
         */
        if (!_body.empty() && !_has_content_type)
            return _fail(INTERNAL_SERVER_ERROR);

        /*
         * The complete CGI body is now known.
         *
         * HEAP is appropriate because the body is held in
         * _body as a std::string.
         */
        _response.setBody(_body,
                          HEAP,
                          _content_type,
                          _headers_only);

        _state = DONE;
        return true;
    }

    State getState() const
    {
        return _state;
    }

    const std::string& getBody() const
    {
        return _body;
    }
};

#endif
