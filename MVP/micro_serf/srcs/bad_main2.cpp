// #include <algorithm>
#include <sys/epoll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <map>
#include <iostream>

// #include "../incs/Server.hpp"
// #include "../incs/Client.hpp"

void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// struct Client {
// 	int fd;
// 	bool writing;
// };

struct Client {
	std::string write_buffer;  // Track unsent data
};

int main() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	set_nonblocking(server_fd);

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {};
	listen(server_fd, 5);

	int epfd = epoll_create1(0);
	epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

	// std::vector<Client> clients;
	std::map<int, Client> clients;
	epoll_event events[100];

	while (true) {
		// Single epoll() call for all I/O
		int nfds = epoll_wait(epfd, events, 100, -1);
		if (nfds < 0) {
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < nfds; ++i) {
			int fd = events[i].data.fd;
			uint32_t revents = events[i].events;

			// Handle server socket (accept connections)
			if (fd == server_fd && (revents & EPOLLIN)) {
				int client_fd = accept(server_fd, NULL, NULL);
				if (client_fd != -1) {
					set_nonblocking(client_fd);
					// clients.push_back({client_fd, false});

					epoll_event ev;
					ev.events = EPOLLIN;  // Start monitoring for read
					ev.data.fd = client_fd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
				}
			}
			// Handle client sockets
			else {
				// Client* client = nullptr;
				// for (auto& c : clients) {
				// 	if (c.fd == fd) {
				// 		client = &c;
				// 		break;
				// 	}
				// }

				// if (!client) continue;
				Client& client = clients[fd];
				clients[fd].write_buffer += "Hello\n";

				// Handle read event
				if (revents & EPOLLIN) {
					char buf[1024];
					ssize_t n = read(fd, buf, sizeof(buf));

					if (n > 0) {
						// Data received
						std::cout << "Read " << n << " bytes from " << fd << "\n";
						// client->writing = true;
						// client->write_buffer = "Hello\n";  // Queue response

						// Modify to monitor for write
						epoll_event ev;
						ev.events = EPOLLIN | EPOLLOUT;
						ev.data.fd = fd;
						epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
					} else {
						// n == 0 (EOF) or n < 0 (error) → close
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						// clients.erase(
						// 	std::find_if(clients.begin(), clients.end(),
						// 		[fd](const Client& c) { return c.fd == fd; }),
						// 	clients.end());
						clients.erase(fd);
					}
				}

				// Handle write event
				// if (revents & EPOLLOUT && client->writing) {
				if (revents & EPOLLOUT && !clients[fd].write_buffer.empty()) {
					// const char* response = "Hello\n";
					// ssize_t n = write(fd, response, std::strlen(response));
					// ssize_t n = write(fd, client->write_buffer.c_str(), client->write_buffer.size());
					ssize_t n = write(fd, clients[fd].write_buffer.c_str(), clients[fd].write_buffer.size());

					if (n > 0) {
						// Successfully wrote
						std::cout << "Wrote " << n << " bytes to " << fd << "\n";
						// client->writing = false;
						// Modify to monitor only read
						// epoll_event ev;
						// ev.events = EPOLLIN;
						// ev.data.fd = fd;
						// epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
						clients[fd].write_buffer.erase(0, n);  // Remove sent bytes
						if (clients[fd].write_buffer.empty()) {
							// All sent, stop monitoring writes
							epoll_event ev;
							ev.events = EPOLLIN;
							ev.data.fd = fd;
							epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
						}
					} else {
						// n < 0 (error) → close
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						// clients.erase(
						// 	std::find_if(clients.begin(), clients.end(),
						// 		[fd](const Client& c) { return c.fd == fd; }),
						// 	clients.end());
						clients.erase(fd);
					}
				}
			}
		}
	}

	close(epfd);
	close(server_fd);
	return 0;
}
