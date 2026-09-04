# Webserver

*This project has been created as part of the 42 curriculum by bstorck, gpochon and sholz.*

# Description

Webserv is an Nginx-style HTTP/1.1 webserver in C++.

- **Event-driven and non-blocking**: built on top of `epoll`, the server can handle many concurrent clients without blocking.
- **Virtual hosting**: one config file can start multiple servers on different ports, each hosting multiple domains with their own locations.
- **Config file parser**: an Nginx-like config syntax with `socket`, `domain` and `location` blocks (see [Config](#config)).
- **Static file serving**: GET and HEAD requests with index files, directory listings (`autoindex`), redirects and custom error pages.
- **File uploads**: POST (multipart) requests, with configurable upload directories (`upload_dir`) and body size limits (`client_max_body_size`).
- **CGI**: scripts (e.g. `.py`, `.sh`) are executed through interpreters configured per file extension.

# Instructions

## Run

To build the webserver run
```
make
```
To start the webserver run
```
./webserv [config_file].conf
```

## Config

### Keywords

The config file has a block structure with Nginx-like directives. Comments start with `#` (full line or inline). Every directive ends with a `;` and blocks are wrapped in `{ }`.

**socket level** (the only allowed top-level block):

| keyword | syntax | description | required |
|---------|--------|-------------|----------|
| `listen` | `listen <port>;` | port to listen on (defaults to `8080`) | ❌ |
| `host` | `host <ip>;` | IPv4 address to bind to | ✅ |
| `client_max_body_size` | `client_max_body_size <bytes>;` | max request body size (default for nested domains/locations) | ❌ |
| `domain` | `domain <name1.com name2.net ...> { }` | server-name block, one per hostname | ❌ |

**domain level:**

| keyword | syntax | description | required |
|---------|--------|-------------|----------|
| `root` | `root <path>;` | document root (the directory is created if it does not exist) | ✅ |
| `index` | `index <file...>;` | default file(s) served for directories (none by default) | ❌ |
| `error_page` | `error_page <status> <path>;` | custom error page for a status code (none by default) | ❌ |
| `client_max_body_size` | `client_max_body_size <bytes>;` | overrides the socket-level limit (default: 16 GiB) | ❌ |
| `location` | `location <path> { }` | path-specific block (any number, can be omitted) | ❌ |

**location level:**

| keyword | syntax | description | required |
|---------|--------|-------------|----------|
| `root` | `root <path>;` | document root for this location | ❌ |
| `alias` | `alias <path>;` | serve files from another directory (incompatible with `root`) | ❌ |
| `redirect` | `redirect <path>;` | redirect requests to another path | ❌ |
| `allow_methods` | `allow_methods <methods...>;` | allowed HTTP methods: `GET POST DELETE` | ❌ |
| `autoindex` | `autoindex <on/off>;` | toggle directory listing | ❌ |
| `index` | `index <file...>;` | overrides the domain index files | ❌ |
| `upload_dir` | `upload_dir <path>;` | where uploaded files (POST/PUT) are saved | ❌ |
| `interpreter` | `interpreter <.ext> <interpreter>;` | CGI interpreter for a file extension | ❌ |
| `error_page` | `error_page <status> <path>;` | overrides/adds domain error pages | ❌ |
| `client_max_body_size` | `client_max_body_size <bytes>;` | overrides domain/socket default | ❌ |

### Example config file

```conf
socket {
	listen 8080;
	host 127.0.0.1;
	client_max_body_size 1048576;

	domain example.com www.example.com {
		root /var/www/example.com/html;
		index index.html;
		error_page 404 /var/www/example.com/errors/404.html;

		location / {
			allow_methods GET;
		}

		location /blog {
			allow_methods GET;
			autoindex on;
		}

		location /uploads {
			alias /var/www/example.com/uploads;
			allow_methods GET POST DELETE;
			upload_dir /tmp;
			autoindex on;
		}

		location /cgi-bin {
			root /var/www/example.com;
			allow_methods GET POST;
			interpreter .py /usr/bin/python3;
			interpreter .sh /bin/bash;
		}
	}
}
```

## View the Webpage

After starting the server, open a browser and visit `http://localhost:<port>`. The port and hostnames are set in the config file.

To use hostnames other than `localhost`, add them to your `/etc/hosts` file pointing at `127.0.0.1`.

# Resources

## AI Models

- Claude
- ChatGPT
- DeepSeek

Sholz used these models to better understand the workflow of the webserver and build a roadmap for its development.
They were also used to help develop concepts and easy-to-use testers during development.

## Other Resources

- https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
- https://developer.mozilla.org/en-US/docs/Learn_web_development/Howto/Web_mechanics/What_is_a_web_server
- https://www.youtube.com/watch?v=YwHErWJIh6Y
- https://youtu.be/3btqIa93FJ4?si=G8qfUgmMgkZvVt2l
