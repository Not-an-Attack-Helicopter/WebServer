# Next Steps — 42 Webserver

## Current Status
Config parsing is complete. `ServerConfig`, `LocationConfig`, and `Config` structs are defined.
`config_parser` loads and validates the config file. `main.cpp` is a stub — the actual server is commented out.

---

## Step 1 — `Server` Class (Socket & Event Loop)

**File:** `src/server.cpp` / `includes/server.hpp`

**Responsibility:** Owns the listening sockets and drives the central `poll`/`epoll` event loop. This is the heart of the server — everything else is called from here.

```cpp
class Server {
private:
    Config                          _config;
    int                             _epoll_fd;
    std::map<int, ServerConfig*>    _listen_fds;   // fd -> which server config
    std::map<int, Client*>          _clients;      // fd -> client state

public:
    Server(const Config& config);
    ~Server();

    void    setup();    // bind + listen for each ServerConfig
    void    run();      // main epoll loop
    void    stop();     // cleanup all fds

private:
    void    _accept_new_client(int listen_fd);
    void    _handle_client_read(int client_fd);
    void    _handle_client_write(int client_fd);
    void    _close_client(int client_fd);
    int     _create_listen_socket(const ServerConfig& sc);
};
```

**Key allowed functions used:**
- `socket`, `bind`, `listen`, `accept`
- `setsockopt` (SO_REUSEADDR, SO_REUSEPORT)
- `fcntl` (set O_NONBLOCK on every fd — required)
- `epoll_create`, `epoll_ctl`, `epoll_wait`

**Notes:**
- Every socket (listening AND client) must be set to `O_NONBLOCK` via `fcntl`.
- `epoll_wait` with a timeout (e.g. 5000 ms) lets you handle hung clients.
- Use `EPOLLIN | EPOLLET` or level-triggered — choose one style and stick to it.
- Multiple `ServerConfig`s on the same port/host must share one listen socket and be disambiguated by `Host:` header later.

---

## Step 2 — `Client` Class (Connection State Machine)

**File:** `src/client.cpp` / `includes/client.hpp`

**Responsibility:** Represents one connected TCP client. Buffers raw bytes coming in, hands them to the request parser, holds the response to be sent out.

```cpp
class Client {
public:
    int             fd;
    ServerConfig*   server;     // which virtual host owns this connection
    std::string     recv_buf;   // raw incoming bytes
    std::string     send_buf;   // serialised response waiting to be written
    time_t          last_active;

    // parsed request + built response live here while being processed
    HttpRequest*    request;
    HttpResponse*   response;

    Client(int fd, ServerConfig* server);
    ~Client();

    bool    read_data();        // recv() into recv_buf, returns false on disconnect
    bool    write_data();       // send() from send_buf, returns false on error
    bool    is_timed_out() const;
    void    reset();            // reuse connection (keep-alive)
};
```

**Key allowed functions used:** `recv`, `send`, `close`

**Notes:**
- Never call `recv`/`send` outside this class.
- Track `last_active` with `time(NULL)` to close stale connections.
- After a full response is written, check `Connection: keep-alive` vs `close`.

---

## Step 3 — `HttpRequest` Class (Request Parser)

**File:** `src/http_request.cpp` / `includes/http_request.hpp`

**Responsibility:** Parses raw bytes from `Client::recv_buf` into structured HTTP/1.1 fields.

```cpp
class HttpRequest {
public:
    std::string                         method;       // GET POST DELETE
    std::string                         uri;          // /path?query
    std::string                         path;         // uri without query string
    std::string                         query;
    std::string                         version;      // HTTP/1.1
    std::map<std::string, std::string>  headers;
    std::string                         body;
    size_t                              content_length;
    bool                                complete;     // full request received

    enum ParseState { REQUEST_LINE, HEADERS, BODY, DONE, ERROR };
    ParseState state;

    HttpRequest();

    // Feed bytes; returns true when a complete request is buffered
    bool    parse(const std::string& raw);

    void    reset();

private:
    bool    _parse_request_line(const std::string& line);
    bool    _parse_header_line(const std::string& line);
    bool    _parse_body(const std::string& raw, size_t header_end);
};
```

**Notes:**
- Use a state machine so partial reads are handled gracefully.
- Validate method, URI length, HTTP version early and store an error code if invalid.
- Enforce `client_max_body_size` from the matched `ServerConfig` before copying body bytes.
- Decode `%xx` percent-encoding in the path.

---

## Step 4 — `HttpResponse` Class (Response Builder)

**File:** `src/http_response.cpp` / `includes/http_response.hpp`

**Responsibility:** Builds the HTTP response that will be written into `Client::send_buf`.

```cpp
class HttpResponse {
public:
    int                                 status_code;
    std::string                         status_text;
    std::map<std::string, std::string>  headers;
    std::string                         body;

    HttpResponse();

    void        set_status(int code);
    void        set_body(const std::string& content, const std::string& mime_type);
    void        set_file_body(const std::string& file_path);  // reads file with open/read
    std::string build() const;   // serialise to wire format

    // Helpers
    static std::string  get_mime_type(const std::string& extension);
    static std::string  get_status_text(int code);
};
```

**Key allowed functions used:** `open`, `read`, `close`, `stat` (for Content-Length)

**Notes:**
- Always set `Content-Length`.
- Serve custom error pages from `ServerConfig::error_pages` when available, else generate a minimal HTML body.
- Add `Date` header using `time` + `gmtime` + `strftime`.

---

## Step 5 — `RequestHandler` / Router

**File:** `src/request_handler.cpp` / `includes/request_handler.hpp`

**Responsibility:** Given a parsed `HttpRequest` and the active `ServerConfig`, match the best `LocationConfig`, enforce method restrictions, and dispatch to the correct handler (static file, redirect, CGI, upload, autoindex).

```cpp
class RequestHandler {
public:
    RequestHandler(const ServerConfig& server, const HttpRequest& req);

    HttpResponse    handle();

private:
    const ServerConfig&     _server;
    const HttpRequest&      _req;
    const LocationConfig*   _location;

    const LocationConfig*   _match_location() const;
    bool                    _method_allowed() const;

    HttpResponse    _handle_redirect();
    HttpResponse    _handle_static();
    HttpResponse    _handle_autoindex(const std::string& dir_path);
    HttpResponse    _handle_upload();
    HttpResponse    _handle_delete();
    HttpResponse    _error_response(int code);
};
```

**Location matching rule:** longest prefix wins (iterate all locations, pick the one whose `path` is the longest prefix of `req.path`).

---

## Step 6 — `AutoIndex` (Directory Listing)

**File:** `src/autoindex.cpp` / `includes/autoindex.hpp`  
(Can also be a static method inside `RequestHandler`.)

**Responsibility:** Generate an HTML page listing directory contents when `autoindex on` and no index file exists.

```cpp
class AutoIndex {
public:
    static std::string generate(const std::string& uri, const std::string& dir_path);
};
```

**Key allowed functions used:** `opendir`, `readdir`, `closedir`, `stat`

---

## Step 7 — `CgiHandler` Class

**File:** `src/cgi_handler.cpp` / `includes/cgi_handler.hpp`

**Responsibility:** Fork a child process, exec the CGI script, pipe the request body in, read the output back, and wrap it in an `HttpResponse`.

```cpp
class CgiHandler {
public:
    CgiHandler(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& srv);

    HttpResponse    execute();  // blocks until child exits (use waitpid)

private:
    const HttpRequest&      _req;
    const LocationConfig&   _loc;
    const ServerConfig&     _srv;

    std::string     _build_script_path() const;
    void            _set_env(char* env[], const std::string& script_path) const;
    std::string     _read_output(int fd) const;
};
```

**Key allowed functions used:** `fork`, `execve`, `pipe`, `dup2`, `waitpid`, `kill`, `access`

**Required CGI env vars to set:**
```
REQUEST_METHOD, CONTENT_TYPE, CONTENT_LENGTH,
SCRIPT_FILENAME, PATH_INFO, QUERY_STRING,
SERVER_NAME, SERVER_PORT, REDIRECT_STATUS
```

**Notes:**
- Use two pipes: one for stdin (body → child), one for stdout (child output → response).
- Set a timeout: after ~5 s send `SIGKILL` to the child and return 504.
- Parse the CGI output for a `Status:` header to override the response code.
- Non-blocking CGI is advanced; for a first pass, make it blocking with `waitpid`.

---

## Step 8 — Virtual Host Resolution

**Where:** In `Server::_accept_new_client` or early in `RequestHandler`.

When multiple `ServerConfig`s listen on the same port, match by `Host:` header against `server_names`. Fall back to the first config that matches the port.

```cpp
ServerConfig* Server::_resolve_virtual_host(int port, const std::string& host_header);
```

---

## Step 9 — Wire Everything Together in `main.cpp`

```cpp
int main(int argc, char** argv) {
    std::string conf_path = (argc >= 2) ? argv[1] : "Config_Files/default.conf";
    config_parser parser(conf_path);
    Server server(parser.get_config());
    server.setup();
    server.run();
    return 0;
}
```

Add a `signal(SIGINT, handler)` that sets a global `g_running = false` flag so `server.run()` exits cleanly and all fds are closed.

---

## Suggested Build Order

| # | Task | Depends on |
|---|------|-----------|
| 1 | `Server` — listen sockets + epoll loop (no real I/O yet) | Config done |
| 2 | `Client` — recv/send buffers | Server |
| 3 | `HttpRequest` — full parser with state machine | Client |
| 4 | `HttpResponse` + `RequestHandler` — static GET only | HttpRequest |
| 5 | Autoindex | RequestHandler |
| 6 | POST upload + DELETE | RequestHandler |
| 7 | Redirects | RequestHandler |
| 8 | CGI | RequestHandler |
| 9 | Virtual host resolution | Server + HttpRequest |
| 10 | Timeout handling + keep-alive | Client + Server |

---

## Allowed Functions Cheat Sheet (42 subject)

| Category | Functions |
|----------|-----------|
| Sockets | `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `setsockopt`, `getsockname`, `socketpair` |
| I/O Multiplex | `select`, `poll`, `epoll_create`, `epoll_ctl`, `epoll_wait` |
| File I/O | `open`, `close`, `read`, `write`, `stat`, `access`, `fcntl` |
| Directory | `opendir`, `readdir`, `closedir` |
| Process | `fork`, `execve`, `waitpid`, `kill`, `signal`, `pipe`, `dup`, `dup2` |
| Network helpers | `getaddrinfo`, `freeaddrinfo`, `getprotobyname`, `htons`, `htonl`, `ntohs`, `ntohl`, `inet_pton`, `inet_ntop` |
| Misc | `strerror`, `gai_strerror`, `errno`, `chdir` |
