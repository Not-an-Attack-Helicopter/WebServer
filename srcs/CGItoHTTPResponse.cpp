#ifndef CGI_HTTP_RESPONSE_HPP
#define CGI_HTTP_RESPONSE_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <limits>

class CgiHttpResponse
{
public:
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

private:
    static bool equalsIgnoreCase(const std::string& a,
                                 const std::string& b)
    {
        if (a.size() != b.size())
            return false;

        std::size_t i;

        for (i = 0; i < a.size(); ++i)
        {
            char ca = a[i];
            char cb = b[i];

            if (ca >= 'A' && ca <= 'Z')
                ca = static_cast<char>(ca - 'A' + 'a');

            if (cb >= 'A' && cb <= 'Z')
                cb = static_cast<char>(cb - 'A' + 'a');

            if (ca != cb)
                return false;
        }

        return true;
    }

    static bool isHopByHopHeader(const std::string& name)
    {
        /*
         * These are controlled by the HTTP layer and are not
         * copied from CGI output.
         */
        return equalsIgnoreCase(name, "Connection") ||
               equalsIgnoreCase(name, "Keep-Alive") ||
               equalsIgnoreCase(name, "Proxy-Authenticate") ||
               equalsIgnoreCase(name, "Proxy-Authorization") ||
               equalsIgnoreCase(name, "TE") ||
               equalsIgnoreCase(name, "Trailer") ||
               equalsIgnoreCase(name, "Transfer-Encoding") ||
               equalsIgnoreCase(name, "Upgrade");
    }

    static bool isServerControlledHeader(const std::string& name)
    {
        return equalsIgnoreCase(name, "Content-Length") ||
               equalsIgnoreCase(name, "Date") ||
               equalsIgnoreCase(name, "Server");
    }

    static std::string statusReason(int status)
    {
        switch (status)
        {
            case 100: return "Continue";
            case 101: return "Switching Protocols";

            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";

            case 300: return "Multiple Choices";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            case 307: return "Temporary Redirect";
            case 308: return "Permanent Redirect";

            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Payload Too Large";
            case 414: return "URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 426: return "Upgrade Required";
            case 428: return "Precondition Required";
            case 429: return "Too Many Requests";
            case 431: return "Request Header Fields Too Large";

            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            case 511: return "Network Authentication Required";
        }

        return "";
    }

    static bool isNoBodyStatus(int status)
    {
        return (status >= 100 && status < 200) ||
               status == 204 ||
               status == 304;
    }

    static bool appendHeader(std::string& response,
                             std::size_t& header_size,
                             const std::string& name,
                             const std::string& value)
    {
        std::string line = name + ": " + value + "\r\n";

        if (header_size > MAX_HEADER_SIZE - line.size())
            return false;

        response += line;
        header_size += line.size();

        return true;
    }

    static bool validHttpVersion(const std::string& version)
    {
        return version == "HTTP/1.1" ||
               version == "HTTP/1.0";
    }

    static bool hasHeader(const std::vector<Header>& headers,
                          const std::string& name)
    {
        std::size_t i;

        for (i = 0; i < headers.size(); ++i)
        {
            if (equalsIgnoreCase(headers[i].name, name))
                return true;
        }

        return false;
    }

public:
    /*
     * Constructs the complete HTTP response.
     *
     * http_version must be "HTTP/1.1" or "HTTP/1.0".
     *
     * status_code:
     *   200 if CGI did not provide Status.
     *
     * status_reason:
     *   CGI Status reason phrase, when supplied.
     *
     * location:
     *   CGI Location value, empty when absent.
     *
     * headers:
     *   CGI response headers.
     *
     * body:
     *   Complete CGI response body.
     *
     * Returns false when the CGI response cannot be converted
     * into a valid response under the server's policy.
     */
    static bool build(const std::string& http_version,
                      int status_code,
                      const std::string& status_reason,
                      bool has_status,
                      const std::string& location,
                      bool has_location,
                      const std::vector<Header>& headers,
                      const std::string& body,
                      std::string& response)
    {
        if (!validHttpVersion(http_version))
            return false;

        /*
         * A CGI Location without Status is a client redirect.
         */
        if (has_location && !has_status)
        {
            status_code = 302;
            status_reason = "Found";
        }
        else if (!has_status)
        {
            status_code = 200;
            status_reason = "OK";
        }

        /*
         * Status codes that cannot carry a response body.
         */
        if (isNoBodyStatus(status_code))
        {
            /*
             * CGI body must not be forwarded for these statuses.
             */
            if (!body.empty())
                return false;
        }

        response.clear();

        /*
         * Status line.
         */
        {
            std::ostringstream out;

            out << http_version
                << " "
                << status_code
                << " ";

            if (!status_reason.empty())
                out << status_reason;
            else
                out << statusReason(status_code);

            out << "\r\n";

            response = out.str();
        }

        std::size_t header_size = response.size();

        /*
         * Copy CGI headers except fields controlled by the HTTP
         * response layer.
         */
        std::size_t i;

        for (i = 0; i < headers.size(); ++i)
        {
            const std::string& name = headers[i].name;
            const std::string& value = headers[i].value;

            /*
             * CGI-only field.
             */
            if (equalsIgnoreCase(name, "Status"))
                continue;

            /*
             * Transfer-Encoding is deliberately not accepted
             * from CGI. The server uses Content-Length because
             * the complete CGI body has been collected.
             */
            if (isHopByHopHeader(name))
                continue;

            /*
             * The server determines Content-Length.
             */
            if (isServerControlledHeader(name))
                continue;

            if (!appendHeader(response,
                              header_size,
                              name,
                              value))
                return false;
        }

        /*
         * CGI Location is an ordinary HTTP response header.
         *
         * It was already present in headers if supplied by CGI.
         * This explicit handling is only needed if the caller's
         * header vector does not contain it.
         */
        if (has_location &&
            !hasHeader(headers, "Location"))
        {
            if (!appendHeader(response,
                              header_size,
                              "Location",
                              location))
                return false;
        }

        /*
         * Server-controlled Content-Length.
         */
        if (!isNoBodyStatus(status_code))
        {
            std::ostringstream out;
            out << static_cast<unsigned long>(body.size());

            if (!appendHeader(response,
                              header_size,
                              "Content-Length",
                              out.str()))
                return false;
        }

        /*
         * HTTP/1.1 persistent connections are the normal case.
         *
         * HTTP/1.0 persistence is not assumed here; the caller
         * can add Connection: keep-alive if its connection policy
         * requires it.
         */
        response += "\r\n";

        /*
         * Body.
         */
        if (!isNoBodyStatus(status_code))
            response += body;

        return true;
    }
};

#endif
