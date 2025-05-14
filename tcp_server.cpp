#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const int PORT = 6380;
const int BACKLOG = 5;

int main() {
  int server_fd, client_fd; // File descriptors
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Socket creation Failed");
    exit(EXIT_FAILURE);
  }
  std::cout << "Socket created Successfully..." << std::endl;

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
  }

  // Binding socket address to an IP and PORT

  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;

  memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(sockaddr)) < 0) {
    perror("Bind Failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  std::cout << "Socket Bound to Port " << PORT << std::endl;

  if (listen(server_fd, BACKLOG) < 0) {
    perror("Listening Failed!");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  std::cout << "Listening on PORT " << PORT << "....." << std::endl;

  while (true) {
    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      perror("Client acception failed.");
      continue;
    }

    std::cout << "Connection accepted from client FD " << client_fd
              << std::endl;

    std::string welcome_msg = "Welcome to Mini Redis";
    send(client_fd, welcome_msg.c_str(), sizeof(welcome_msg), 0);

    close(client_fd);
    std::cout << "Connection Closed for cliet FD : " << client_fd << std::endl;
  }
  close(server_fd);
  return 0;
}
