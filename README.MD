*This project has been created as part of the 42 Luxembourg curriculum by bstorck, gpochon and sholz.*

# 42 Webserv

## Description

Webserv is about creating an HTTP/1.1 server from scratch in C++98. The project's objective is for us to implement and understand the main mechanisms behind a web server: what actually happens between a browser and a web server, how a connection is created, how HTTP requests are received and parsed, and how the server decides what response to send back. Neither Apache nor NGINX needs to be recreated entirely for this project.

Our server can serve static files such as HTML pages, CSS, images and other resources, but also handle uploads, redirects, directory listing, custom error pages and CGI scripts. It can listen on multiple ports and host multiple domains depending on its configuration.

One of the main constraints of the project is that the server needs to stay non-blocking. Instead of creating one thread or process for every client, Webserv uses `epoll()` to monitor sockets and CGI pipes and reacts only when they are ready to be read from or written to.

## Instructions

### Prerequisites

* Linux, since the server uses `epoll()`
* A C++98 compatible compiler
* `make`

### Compilation

Use the Makefile to compile the project:

```bash
make
```

Other available commands:

| Command       | Description                                    |
| ------------- | ---------------------------------------------- |
| `make`        | Compile Webserv                                |
| `make clean`  | Remove object files                            |
| `make fclean` | Remove object files and the Webserv executable |
| `make re`     | Fully recompile the project                    |

### Running Webserv

The server can be started with:

```bash
./webserv [configuration_file]
```

For example:

```bash
./webserv configs/default.conf
```

If no configuration file is given, Webserv uses `configs/default.conf`.

The server also supports a few optional logging arguments:

```bash
./webserv -l DEBUG configs/default.conf
./webserv --log-level debug configs/default.conf
./webserv -q configs/default.conf
./webserv --quiet configs/default.conf
```

### Configuration

Webserv uses its own configuration file format inspired by NGINX.

The configuration defines things such as:

* The IP addresses and ports the server listens on
* Domain names
* Website root directories
* Allowed HTTP methods
* Redirects
* Directory listing
* Default index files
* Upload directories
* Maximum request body size
* CGI interpreters

`configs/default.conf` contains a complete example of the available configuration.

For virtual server hosting, the domains need to be added to your hostfile. All domains should resolve to localhost i.e. 127.0.0.1.

For CGI locations, the interpreter path needs to exist on your machine. You can find it using commands such as:

```bash
which python3
which perl
which bash
```

and update the corresponding paths in the configuration file if needed.

Once the server is running, open the configured address in your browser to access the website.

## Architecture

### From `epoll()` to the response

Webserv is built around a single `epoll()` instance. Instead of constantly checking every connected client, `epoll_wait()` tells the server which file descriptors are currently ready for an operation.

Those file descriptors can represent:

* A listening socket waiting for a new connection
* A client socket ready to receive a request
* A client socket ready to send a response
* A CGI pipe ready to receive or return data

When an event happens, the server determines what kind of file descriptor triggered it and dispatches it to the appropriate handler.

```text
epoll_wait()
     |
     +--> Listening socket --> accept new client
     |
     +--> Client readable --> receive HTTP data
     |
     +--> Client writable --> send HTTP response
     |
     +--> CGI pipe --> read/write CGI data
```

### Client connections

When a browser connects to one of the server's domains, the server accepts the connection and creates a `Client` object for it.

The `Client` keeps track of everything related to that connection, including:

* Data received from the socket
* Data waiting to be sent
* The current HTTP request
* The HTTP response being prepared
* The current state of the connection

A client doesn't necessarily send an entire HTTP request in one operation. The server might receive:

```text
GET /index.ht
```

and only receive:

```text
ml HTTP/1.1
Host: localhost
```

during a later event.

For this reason, requests are parsed incrementally. Webserv keeps the data it has already received and continues parsing when more becomes available.

### Request lifecycle

A normal static request roughly follows this path:

```text
Client socket
     |
     v
Receive bytes
     |
     v
Parse request
     |
     v
Build HTTPRequest object
     |
     v
Match configuration
     |
     v
Find requested resource
     |
     v
Build HTTPResponse object
     |
     v
Send response through socket
```

The request first goes through the parser, which extracts the method, URI, HTTP version, headers and request body when one exists.

Once the request is complete, Webserv uses its configuration to determine which domain and location should handle it.

It can then decide what needs to happen next. Depending on the configuration and request, this can mean serving a static file, returning a redirect, handling an upload, generating a directory listing, deleting a resource or starting a CGI process.

### Why use `epoll()`?

A simple server could accept one client, wait until its complete request arrives, send a response and only then move on to the next client.

The problem is that network operations can be slow. One client taking several seconds to send its request would block every other client during that time.

Webserv instead uses non-blocking sockets. If an operation cannot continue immediately, the server moves on and waits for `epoll()` to notify it when that file descriptor becomes ready again.

This allows a single event loop to manage many connections without creating one thread or process for every client.

### CGI

When the resolved location matches a configured CGI extension, Webserv does not serve the file directly. Instead it runs the file as a script and turns its output into the HTTP response.

```text
Request matches a CGI extension
     |
     v
setUpCGI() builds argv and envp from the request
     |
     v
CGIProcess opens its pipes, then spawn()s (fork + execve)
     |
     v
stdin/stdout pipes are registered with the single epoll() instance
     |
     v
Request body is written to the pipe as it becomes writable
     |
     v
Script output is read as it becomes readable, headers parsed as they arrive
     |
     v
buildResponse() turns the parsed output into an HTTPResponse
```

A `CGIProcess` opens its pipes as soon as it is created. This is cheap and easy to undo if something fails right after. It does not fork until `spawn()` is called, once its stdin and stdout file descriptors are registered with the server's `epoll()` instance. From there it is driven the same way a client socket is. `handleWritable()` writes another chunk of the request body into the pipe when it is ready to accept more. `handleReadable()` drains whatever the script has written so far, and the response headers are parsed line by line directly from that incoming data instead of waiting for the whole script to finish. A CGI process that runs past its deadline is killed and reaped without blocking the rest of the server.

A slow or stuck CGI script behaves like a slow client. It only ties up its own pipes, never the whole event loop.

### Sessions and cookies

Webserv can keep track of a client across multiple requests using a session cookie.

When a request comes in, Webserv reads its `Cookie` header and looks for a `Session_ID` value. If that ID matches a session that is still valid, the session's timer is refreshed and the request continues. If the ID is missing, unknown, or the session expired, Webserv creates a new `Session`, generates a random hex ID for it with `randomHexString()`, and stores it in the `SessionManager`. The response then gets a `Set-Cookie` header so the browser sends that same ID back on its next request.

Each session keeps a creation time, a last touched time, and an expiration time. By default a session lives for one hour of inactivity. Expired sessions are removed from memory by a periodic sweep instead of being kept around forever.

Sessions are plain in memory objects tied to a random ID, not anything the client can read or change directly. All it holds on the client side is the cookie.

TLDR; `epoll()` tells Webserv where something can currently happen, the `Client` stores the state of each connection, and the server progressively moves each request from receiving data to parsing, processing and finally sending a response.

## Testing

The `configs/` folder has configuration files meant to be used for testing, split into three groups:

* `configs/normal/` contains valid configurations covering common setups such as static sites, uploads, redirects and CGI.
* `configs/invalid/` contains configurations that should be rejected, one broken directive at a time.
* `configs/limits/` contains configurations that push boundaries such as port numbers, number of servers, or body size limits.

Run the server against any of them, for example:

```bash
./webserv configs/normal/normal_01_basic_static.conf
```

Then test it with a browser or `curl`.

For stress testing, tools such as `siege` are useful to check that Webserv keeps answering requests and does not leak memory or file descriptors under sustained load:

```bash
siege -b http://127.0.0.1:8080/
```

Let `siege` run for a while, then stop it and check that the server still answers normal requests afterward.
