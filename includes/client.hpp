#pragma once
#include "webserver.hpp"

class Client {
private:
    int             _fd;
    const ServerConfig* _server;
    std::string     _recv_buf;
    std::string     _send_buf;
    time_t          _last_active;
    ParseState      _parse_state;
    HTTPRequest     _request;
    HTTPResponse    _response;

    Client();

public:
    Client(int fd, const ServerConfig* server);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();

    // Accessors
    int             getFd()         const;
    const ServerConfig* getServer()     const;
    time_t          getLastActive() const;
    ParseState      getParseState() const;
    void            setParseState(ParseState state);

    // Buffer access
    const std::string&  getRecvBuf()    const;
    void                clearRecvBuf();
    void                setSendBuf(const std::string& data);
    bool                isSendBufEmpty() const;

    // Request / response
    HTTPRequest&        getRequest();
    const HTTPRequest&  getRequest()  const;
    HTTPResponse&       getResponse();

    // I/O
    bool    read_data();        // recv() into _recv_buf, returns false on disconnect
    bool    write_data();       // send() from _send_buf, returns false on error
    bool    is_timed_out()  const;
    void    reset();            // reuse connection (keep-alive)
};