#include "utils.h"
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

using std::cout;
using std::endl;
using utils::send_response;

const int PORT = 6380;
const int BACKLOG = 5;
const int BUFFER_SIZE = 1024;

std::string DUMP_FILE_NAME = "miniredis.dump";
std::unordered_map<std::string, std::string> data_store;
std::mutex data_store_mutex;

bool save_to_disk() {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  std::ofstream outfile(DUMP_FILE_NAME, std::ios::out | std::ios::trunc);

  if (!outfile.is_open()) {
    std::cerr << "Error!, Couldn't open dump file " << DUMP_FILE_NAME
              << "for writing" << endl;
    return false;
  }

  cout << "Saving data to " << DUMP_FILE_NAME << " ...." << endl;
  for (const auto &pair : data_store) {
    outfile << pair.first << endl;
    outfile << pair.second << endl;
  }
  outfile.close();
  cout << "Data saved Successfully." << endl;
  return true;
}

bool load_from_disk() {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  std::ifstream infile(DUMP_FILE_NAME, std::ios::in);

  if (!infile.is_open()) {
    std::cerr << "Error!, Couldn't open or can't find dump file "
              << DUMP_FILE_NAME << "for reading starting with empty store"
              << endl;
    return false;
  }

  cout << "Loading data from " << DUMP_FILE_NAME << "...." << endl;
  data_store.clear();

  std::string key, value;
  int keys_loaded = 0;

  while (std::getline(infile, key) && std::getline(infile, value)) {
    data_store[key] = value;
    keys_loaded++;
  }

  if (keys_loaded > 0) {
    cout << "Data loaded Successfully. Keys Loaded : " << keys_loaded << endl;
  } else if (infile.eof() && keys_loaded == 0 && data_store.empty()) {
    cout << "Dump file  " << DUMP_FILE_NAME
         << " is empty or got formatting issue" << endl;
  } else if (!infile.eof()) {
    std::cerr << "Error reading from dump file. Data might be corrupted"
              << endl;
    infile.close();
    return false;
  }
  infile.close();
  return true;
}

void handle_client(int client_fd) {
  cout << "Thread : " << std::this_thread::get_id() << " Handling Client FD"
       << client_fd << endl;
  char buffer[BUFFER_SIZE];
  std::string accumulated_string;

  while (true) {
    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_recieved <= 0) {
      if (bytes_recieved == 0) {
        cout << "Thread : " << std::this_thread::get_id()
             << " Client FD : " << client_fd << "Disconnected" << endl;
      } else {
        if (errno != ECONNRESET && errno != EPIPE) {
          perror(("recv failed for client" +
                  std::to_string(reinterpret_cast<uintptr_t>(pthread_self())) +
                  ": recv failed")
                     .c_str());
        } else {
          cout << "Thread " << std::this_thread::get_id()
               << ": Client FD : " << client_fd
               << "Connection reset or epipe problem" << endl;
        }
      }
      close(client_fd);
      return;
    }

    // Some data recieved

    buffer[bytes_recieved] = '\0';
    accumulated_string += buffer;
    cout << accumulated_string;

    size_t newline_pos;
    while ((newline_pos = accumulated_string.find('\n')) != std::string::npos) {
      std::string command_line = accumulated_string.substr(0, newline_pos);
      accumulated_string.erase(0, newline_pos + 1);

      if (!command_line.empty() && command_line.back() == '\r') {
        command_line.pop_back();
      }

      cout << "Client FD : " << client_fd << "Sent : " << command_line << endl;

      if (command_line.empty())
        continue;

      std::vector<std::string> tokens = utils::tokenize(command_line);
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
        utils::kv_set(tokens[1], tokens[2], data_store, data_store_mutex);
        send_response(client_fd, "+OK\r\n");
      } else if (command == "GETALL" && tokens.size() == 1) {
        auto all_data = utils::kv_getall(data_store, data_store_mutex);
        if (all_data) {
          for (const auto &[key, value] : *all_data) {
            send_response(client_fd, key + " : " + value + "\r\n");
          }
        } else {
          send_response(client_fd, "$-1\r\n");
        }
      } else if (command == "GET" && tokens.size() == 2) {
        std::optional<std::string> value =
            utils::kv_get(tokens[1], data_store, data_store_mutex);
        if (value)
          send_response(client_fd, "$" + std::to_string(value->length()) +
                                       "\r\n" + *value + "\r\n");
        else
          send_response(client_fd, "$-1\r\n");
      } else if (command == "DEL" && tokens.size() == 2) {
        if (utils::kv_del(tokens[1], data_store, data_store_mutex))
          send_response(client_fd, ":1\r\n");
        else
          send_response(client_fd, ":0\r\n");
      } else if (command == "SAVE") {
        if (save_to_disk()) {
          send_response(client_fd, "+OK\r\n");
        }
      } else if (command == "QUIT") {
        send_response(client_fd, "+OK\r\n");
        cout << "Client FD : " << client_fd << "is Quitting......." << endl;
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
  cout << "Socket created Successfully..." << endl;

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
  cout << "Socket Bound to Port " << PORT << endl;

  if (listen(server_fd, BACKLOG) < 0) {
    perror("Listening Failed!");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  if (!load_from_disk()) {
    // Error handling
  }
  cout << "Listening on PORT " << PORT << "....." << endl;

  while (true) {
    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      perror("Client acception failed.");
      continue;
    }

    cout << "Connection accepted from client FD " << client_fd << endl;

    std::thread client_thread(handle_client, client_fd);
    client_thread.detach();
  }
  close(server_fd);
  return 0;
}
