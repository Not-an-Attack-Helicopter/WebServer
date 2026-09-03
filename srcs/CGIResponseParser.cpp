#ifndef CGI_RESPONSE_PARSER_HPP
#define CGI_RESPONSE_PARSER_HPP

#include <string>
#include <vector>
// #include <utility>
#include <cstdlib>
#include <cctype>
// #include <stdexcept>

class CgiResponseParser
{
public:
    enum State
    {
        PARSING_HEADERS,
        HEADERS_COMPLETE,
        COMPLETE,
        ERROR
    };

    struct Header
    {
        std::string name;
        std::string value;

        Header() {}

        Header(const std::string& n, const std::string& v)
            : name(n), value(v) {}
    };

private:
    static const std::size_t MAX_HEADER_SIZE = 32 * 1024;

    State               _state;
    std::size_t         _header_bytes;

    std::vector<Header>  _headers;
    std::string          _line;

    bool                 _has_content_type;
    bool                 _has_location;
    bool                 _has_status;

    int                  _status_code;
    std::string          _status_reason;
    std::string          _content_type;
    std::string          _location;

    /*
     * Bytes which were received after the CGI header terminator.
     * These are the first bytes of the response body.
     */
    std::string          _body;

private:
    static bool isTokenChar(unsigned char c)
    {
        /*
         * RFC 3875 token:
         *
         * CHAR excluding CTLs and separators.
         */
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

    static bool isFieldName(const std::string& s)
    {
        if (s.empty())
            return false;

        std::size_t i;

        for (i = 0; i < s.size(); ++i)
        {
            if (!isTokenChar(
                    static_cast<unsigned char>(s[i])))
                return false;
        }

        return true;
    }

    static bool equalsIgnoreCase(const std::string& a,
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

    static std::string trimOWS(const std::string& s)
    {
        std::size_t first = 0;
        std::size_t last = s.size();

        while (first < last &&
               (s[first] == ' ' || s[first] == '\t'))
            ++first;

        while (last > first &&
               (s[last - 1] == ' ' ||
                s[last - 1] == '\t'))
            --last;

        return s.substr(first, last - first);
    }

    static bool isThreeDigits(const std::string& s)
    {
        return s.size() == 3 &&
               std::isdigit(
                   static_cast<unsigned char>(s[0])) &&
               std::isdigit(
                   static_cast<unsigned char>(s[1])) &&
               std::isdigit(
                   static_cast<unsigned char>(s[2]));
    }

    static int parseStatusCode(const std::string& value,
                               std::string& reason)
    {
        /*
         * Status = 3DIGIT SP reason-phrase
         */

        if (value.size() < 5)
            return -1;

        if (!isThreeDigits(value.substr(0, 3)))
            return -1;

        if (value[3] != ' ')
            return -1;

        reason = value.substr(4);

        /*
         * reason-phrase = *TEXT
         *
         * RFC 3875's TEXT is printable. Reject CTLs here.
         */
        std::size_t i;

        for (i = 0; i < reason.size(); ++i)
        {
            unsigned char c =
                static_cast<unsigned char>(reason[i]);

            if (c < 32 || c == 127)
                return -1;
        }

        return (value[0] - '0') * 100 +
               (value[1] - '0') * 10 +
               (value[2] - '0');
    }

    bool parseHeaderLine(const std::string& line)
    {
        /*
         * Continuation lines are explicitly forbidden by CGI/1.1.
         */
        if (!line.empty() &&
            (line[0] == ' ' || line[0] == '\t'))
            return false;

        std::string::size_type colon = line.find(':');

        if (colon == std::string::npos)
            return false;

        std::string name = line.substr(0, colon);

        if (!isFieldName(name))
            return false;

        std::string value;

        if (colon + 1 < line.size())
            value = trimOWS(line.substr(colon + 1));

        /*
         * Every CGI field may occur at most once.
         */
        if (equalsIgnoreCase(name, "Content-Type"))
        {
            if (_has_content_type)
                return false;

            _has_content_type = true;
            _content_type = value;
        }
        else if (equalsIgnoreCase(name, "Location"))
        {
            if (_has_location)
                return false;

            _has_location = true;
            _location = value;
        }
        else if (equalsIgnoreCase(name, "Status"))
        {
            if (_has_status)
                return false;

            _has_status = true;

            std::string reason;
            int code = parseStatusCode(value, reason);

            if (code < 0)
                return false;

            _status_code = code;
            _status_reason = reason;
        }

        _headers.push_back(Header(name, value));
        return true;
    }

    bool finishHeaders()
    {
        /*
         * At least one CGI field must be present.
         */
        if (!_has_content_type &&
            !_has_location &&
            !_has_status)
            return false;

        /*
         * If there is a body, Content-Type will ultimately
         * be required. At this point we don't yet know whether
         * a body will follow, so that check is deferred.
         */

        _state = HEADERS_COMPLETE;
        return true;
    }

    bool processLine()
    {
        /*
         * Empty line terminates the CGI header section.
         */
        if (_line.empty())
        {
            if (!finishHeaders())
            {
                _state = ERROR;
                return false;
            }

            return true;
        }

        if (!parseHeaderLine(_line))
        {
            _state = ERROR;
            return false;
        }

        _line.clear();
        return true;
    }

public:
    CgiResponseParser()
        : _state(PARSING_HEADERS),
          _header_bytes(0),
          _has_content_type(false),
          _has_location(false),
          _has_status(false),
          _status_code(200)
    {
    }

    State state() const
    {
        return _state;
    }

    bool hasHeaders() const
    {
        return _state == HEADERS_COMPLETE ||
               _state == COMPLETE;
    }

    bool hasBody() const
    {
        return !_body.empty();
    }

    bool hasContentType() const
    {
        return _has_content_type;
    }

    bool hasLocation() const
    {
        return _has_location;
    }

    bool hasStatus() const
    {
        return _has_status;
    }

    int statusCode() const
    {
        return _has_status ? _status_code : 200;
    }

    const std::string& statusReason() const
    {
        return _status_reason;
    }

    const std::string& contentType() const
    {
        return _content_type;
    }

    const std::string& location() const
    {
        return _location;
    }

    const std::vector<Header>& headers() const
    {
        return _headers;
    }

    const std::string& body() const
    {
        return _body;
    }

    /*
     * Feed one chunk obtained from cgi_stdout.
     *
     * Returns false only when the CGI response is invalid
     * according to the parser's rules.
     *
     * Once headers are complete, all subsequent bytes are
     * body bytes.
     */
    bool feed(const char* data, std::size_t size)
    {
        if (_state == ERROR)
            return false;

        if (_state == COMPLETE)
            return false;

        std::size_t i = 0;

        while (i < size)
        {
            if (_state == HEADERS_COMPLETE)
            {
                _body.append(data + i, size - i);
                return true;
            }

            char c = data[i++];

            /*
             * Header size includes all bytes consumed as
             * part of the CGI header section, including
             * newline bytes.
             */
            ++_header_bytes;

            if (_header_bytes > MAX_HEADER_SIZE)
            {
                _state = ERROR;
                return false;
            }

            if (c == '\n')
            {
                /*
                 * Accept:
                 *
                 *   LF
                 *   CRLF
                 *
                 * Internally remove the preceding CR when
                 * present.
                 */
                if (!_line.empty() &&
                    _line[_line.size() - 1] == '\r')
                {
                    _line.erase(_line.size() - 1);
                }

                if (!processLine())
                    return false;
            }
            else
            {
                /*
                 * A bare CR is not itself a newline.
                 */
                _line += c;
            }
        }

        return true;
    }

    /*
     * Called when EOF is received from cgi_stdout.
     *
     * A CGI response must have supplied a complete header
     * section before the script terminates.
     */
    bool finish()
    {
        if (_state == ERROR)
            return false;

        if (_state == PARSING_HEADERS)
        {
            _state = ERROR;
            return false;
        }

        /*
         * A document response with a body requires
         * Content-Type.
         */
        if (!_body.empty() &&
            !_has_content_type)
        {
            _state = ERROR;
            return false;
        }

        _state = COMPLETE;
        return true;
    }
};

#endif
