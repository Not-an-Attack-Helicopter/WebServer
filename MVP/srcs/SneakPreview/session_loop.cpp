void handleClient(int clientSocket, SessionManager& sessionManager) {
  char buffer[4096];
  ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
  
  if (bytesRead > 0) {
    buffer[bytesRead] = '\0';
    std::string requestData(buffer);
    
    // Parse request (stateless)
    std::string method, path, sessionId;
    RequestHandler::parseRequest(requestData, method, path, sessionId);
    
    HTTPResponse response;
    
    // Validate session and build response (stateless + manager)
    if (sessionId.empty()) {
      // No session cookie, create new one
      std::string newSessionId = sessionManager.createSession();
      RequestHandler::buildNewSessionResponse(newSessionId, response);
    } else {
      // Check if session exists and is valid
      std::string userData;
      if (sessionManager.getSessionData(sessionId, userData, "user")) {
        RequestHandler::buildExistingSessionResponse(userData, response);
      } else {
        // Session expired or doesn't exist
        std::string newSessionId = sessionManager.createSession();
        RequestHandler::buildExpiredSessionResponse(newSessionId, response);
      }
    }
    
    std::string responseStr = response.serialize();
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
  }
  
  close(clientSocket);
}

int main() {
  SessionManager sessionManager;
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  
  // ... bind, listen setup ...
  
  int epollFd = epoll_create1(0);
  epoll_event ev, events[MAX_EVENTS];
  
  ev.events = EPOLLIN;
  ev.data.fd = serverSocket;
  epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &ev);
  
  while (true) {
    int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == serverSocket) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        ev.events = EPOLLIN;
        ev.data.fd = clientSocket;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &ev);
      } else {
        handleClient(events[i].data.fd, sessionManager);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
      }
    }
  }
}
