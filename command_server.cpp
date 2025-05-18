#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

const int PORT = 6380;
const int BACKLOG = 5;
const int BUFFER_SIZE = 1024;
std::string DUMP_FILE_NAME = "miniredis.dump";
std::unordered_map<std::string, std::string> data_store;
std::mutex data_store_mutex;

bool kv_del(const std::string &key) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  return data_store.erase(key) > 0;
}

bool save_to_disk() {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  std::ofstream outfile(DUMP_FILE_NAME, std::ios::out | std::ios::trunc);

  if (!outfile.is_open()) {
    std::cerr << "Error!, Couldn't open dump file " << DUMP_FILE_NAME
              << "for writing" << std::endl;
    return false;
  }

  std::cout << "Saving data to " << DUMP_FILE_NAME << " ...." << std::endl;
  for (const auto &pair : data_store) {
    outfile << pair.first << std::endl;
    outfile << pair.second << std::endl;
  }
  outfile.close();
  std::cout << "Data saved Successfully." << std::endl;
  return true;
}

bool load_from_disk() {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  std::ifstream infile(DUMP_FILE_NAME, std::ios::in);

  if (!infile.is_open()) {
    std::cerr << "Error!, Couldn't open or can't find dump file "
              << DUMP_FILE_NAME << "for reading starting with empty store"
              << std::endl;
    return false;
  }

  std::cout << "Loading data from " << DUMP_FILE_NAME << "...." << std::endl;
  data_store.clear();

  std::string key, value;
  int keys_loaded = 0;

  while (std::getline(infile, key) && std::getline(infile, value)) {
    data_store[key] = value;
    keys_loaded++;
  }

  if (keys_loaded > 0) {
    std::cout << "Data loaded Successfully." << std::endl;
  } else if (infile.eof() && keys_loaded == 0 && data_store.empty()) {
    std::cout << "Dump file  " << DUMP_FILE_NAME
              << " is empty or got formatting issue" << std::endl;
  } else if (!infile.eof()) {
    std::cerr << "Error reading from dump file. Data might be corrupted"
              << std::endl;
    infile.close();
    return false;
  }
  infile.close();
  return true;
}

void kv_set(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  data_store[key] = value;
}

std::optional<std::string> kv_get(const std::string &key) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
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
  std::cout << "Thread : " << std::this_thread::get_id()
            << " Handling Client FD" << client_fd << std::endl;
  char buffer[BUFFER_SIZE];
  std::string accumulated_string;

  while (true) {
    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_recieved <= 0) {
      if (bytes_recieved == 0) {
        std::cout << "Thread : " << std::this_thread::get_id()
                  << " Client FD : " << client_fd << "Disconnected"
                  << std::endl;
      } else {
        if (errno != ECONNRESET && errno != EPIPE) {
          perror(("recv failed for client" +
                  std::to_string(reinterpret_cast<uintptr_t>(pthread_self())) +
                  ": recv failed")
                     .c_str());
        } else {
          std::cout << "Thread " << std::this_thread::get_id()
                    << ": Client FD : " << client_fd
                    << "Connection reset or epipe problem" << std::endl;
        }
      }
      close(client_fd);
      return;
    }

    // Some data recieved

    buffer[bytes_recieved] = '\0';
    accumulated_string += buffer;
    std::cout << accumulated_string;

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
      } else if (command == "SAVE") {
        if (save_to_disk()) {
          send_response(client_fd, "+OK\r\n");
        }
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
  if (!load_from_disk()) {
    // Error handling
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

    std::thread client_thread(handle_client, client_fd);
    client_thread.detach();
  }
  close(server_fd);
  return 0;
}
