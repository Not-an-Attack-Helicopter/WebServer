#include "../includes/server.hpp"

Server::Server(const Config& config) : _config(config), _epoll_fd(-1) {
	// Create epoll instance
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1) {
		throw std::runtime_error("Failed to create epoll instance");
	}
}

Server::Server(const Server& other) : _config(other._config), _epoll_fd(-1) {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1) {
		throw std::runtime_error("Failed to create epoll instance");
	}
}

Server& Server::operator=(const Server& other) {
	if (this != &other) {
		_config = other._config;
	}
	return *this;
}

Server::~Server() {
	stop();
	if (_epoll_fd != -1) {
		close(_epoll_fd);
	}
}

void Server::setup()
{

}

void Server::start()
{

}

void Server::stop()
{

}