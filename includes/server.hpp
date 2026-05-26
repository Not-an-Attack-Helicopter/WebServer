#pragma once
#include "webserver.hpp"

class Server {
private:
    Config                          _config;
    int                             _epoll_fd;
    std::map<int, ServerConfig*>    _listen_fds;   // fd -> which server config
    std::map<int, Client*>          _clients;      // fd -> client state

	void    _accept_new_client(int listen_fd);
    void    _handle_client_read(int client_fd);
    void    _handle_client_write(int client_fd);
    void    _close_client(int client_fd);
    int     _create_listen_socket(const ServerConfig& sc);
    Server();   // not implemented — prevents default construction (C++98 = delete)

public:
    Server(const Config& config);
	Server(const Server& other);
	Server& operator=(const Server& other);
    ~Server();

    void    setup();    // bind + listen for each ServerConfig
    void    start();      // main epoll loop
    void    stop();     // cleanup all fds

};