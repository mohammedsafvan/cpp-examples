#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

const int PORT = 6380;
const int BACKLOG = 5;
const int BUFFER_SIZE = 1024;

std::unordered_map<std::string, std::string> data_store;
bool kv_del(const std::string &key) { return data_store.erase(key) > 0; }
void kv_set(const std::string &key, const std::string &value) {
  data_store[key] = value;
}
std::optional<std::string> kv_get(const std::string &key) {
  auto it = data_store.find(key);
  if (it != data_store.end()) {
    return it->second;
  }
  return std::nullopt;
}
std::vector<std::string> tokenize(const std::string &str,
                                  char delimiter = ' ') {
  std::vector<std::string> tokens;
  std::string token;

  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}

void send_response(int client_fd, const std::string &resp) {
  send(client_fd, resp.c_str(), resp.length(), 0);
}

void handle_client(int client_fd) {
  char buffer[BUFFER_SIZE];
  std::string accumulated_string;

  while (true) {
    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_recieved <= 0) {
      if (bytes_recieved == 0) {
        std::cout << "Client FD : " << client_fd << "Disconnected" << std::endl;
      } else {
        perror("recv failed for client");
      }
      close(client_fd);
      return;
    }

    // Some data recieved

    buffer[bytes_recieved] = '\0';
    accumulated_string += buffer;

    size_t newline_pos;
    while ((newline_pos = accumulated_string.find('\n')) != std::string::npos) {
      std::string command_line = accumulated_string.substr(0, newline_pos);
      accumulated_string.erase(0, newline_pos + 1);

      if (!command_line.empty() && command_line.back() == '\r') {
        command_line.pop_back();
      }

      std::cout << "Client FD : " << client_fd << "Sent : " << command_line
                << std::endl;

      if (command_line.empty())
        continue;

      std::vector<std::string> tokens = tokenize(command_line);
      if (tokens.empty()) {
        send_response(client_fd, "-ERR Empty command\r\n");
        continue;
      }

      std::string command = tokens[0];

      for (char &c : command)
        c = toupper(c);

      if (command == "PING") {
        if (tokens.size() == 1)
          send_response(client_fd, "+PONG\r\n");
        else if (tokens.size() == 2)
          send_response(client_fd, "$" + std::to_string(tokens[1].length()) +
                                       "\r\n" + tokens[1] + "\r\n");
        else
          send_response(client_fd,
                        "-ERR wrong number of arguments for PING command");
      } else if (command == "SET" && tokens.size() == 3) {
        kv_set(tokens[1], tokens[2]);
        send_response(client_fd, "+OK\r\n");
      } else if (command == "GET" && tokens.size() == 2) {
        std::optional<std::string> value = kv_get(tokens[1]);
        if (value)
          send_response(client_fd, "$" + std::to_string(value->length()) +
                                       "\r\n" + *value + "\r\n");
        else
          send_response(client_fd, "$-1\r\n");
      } else if (command == "DEL" && tokens.size() == 2) {
        if (kv_del(tokens[1]))
          send_response(client_fd, ":1\r\n");
        else
          send_response(client_fd, ":0\r\n");
      } else if (command == "QUIT") {
        send_response(client_fd, "+OK\r\n");
        std::cout << "Client FD : " << client_fd << "is Quitting......."
                  << std::endl;
        close(client_fd);
        return;
      } else {
        send_response(client_fd,
                      "-ERR Wrong command or wrong number of arguments\r\n");
      }
    }
  }
}

int main() {
  int server_fd, client_fd;
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

    handle_client(client_fd);
  }
  close(server_fd);
  return 0;
}
