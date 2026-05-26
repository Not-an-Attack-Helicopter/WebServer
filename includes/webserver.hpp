#pragma once

/*******************************
 * 📚 C++ Standard Library
 *******************************/
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <cstdarg>

/*******************************
 * 📦 STL Containers & Algorithms
 *******************************/
#include <map>
#include <set>
#include <vector>
#include <list>
#include <algorithm>
#include <iterator>

/*******************************
 * 📁 File / filesystem (POSIX + C)
 *******************************/
#include <fcntl.h>      // open, fcntl
#include <unistd.h>     // read, write, close, dup, dup2, pipe, fork, execve, access, chdir
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // stat
#include <cstring>      // strerror, memset, memcpy
#include <fstream>

/*******************************
 * ⚙️ Process / signals
 *******************************/
#include <sys/types.h>
#include <sys/wait.h>   // waitpid
#include <signal.h>     // kill, signal
#include <errno.h>      // errno

/*******************************
 * 🌐 Networking (POSIX sockets)
 *******************************/
#include <sys/socket.h>  // socket, socketpair, bind, connect, accept, listen, send, recv, setsockopt, getsockname
#include <netinet/in.h>  // htons, htonl, ntohs, ntohl
#include <arpa/inet.h>   // inet_pton, inet_ntop
#include <netdb.h>       // getaddrinfo, freeaddrinfo, getprotobyname, gai_strerror

/*******************************
 * 🔄 I/O Multiplexing
 *******************************/
#include <sys/select.h>  // select
#include <poll.h>        // poll
#include <sys/time.h>    // timeval
#include <sys/epoll.h>   // epoll_create, epoll_ctl, epoll_wait

/*******************************
 * 👤 User defined headers
 *******************************/
#include "types.hpp" //
#include "config_parser.hpp"
#include "utils.hpp"
#include "parse_helper.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "client.hpp"
#include "server.hpp"
