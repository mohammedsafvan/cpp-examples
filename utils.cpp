#include "utils.h"
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <utility>
#include <vector>

using std::cerr;
using std::endl;
using std::string;

using StringMap = std::unordered_map<string, string>;
using ListOfStringPairOrNothing =
    std::optional<std::vector<std::pair<std::string, std::string>>>;

namespace utils {

void play_sound() {
  int res = system("paplay /usr/share/sounds/freedesktop/stereo/bell.oga &");
  if (res != 0) {
    cerr << "Warning: Could not play sound alarm. Is 'paplay' installed "
            "and a sound file available?"
         << endl;
  }
}

void send_dialog(const string &msg) {
  string command = "kdialog --title=\"TermAlarm\" --msgbox \"Time's up\"";
  int res = system(command.c_str());
  if (res != 0) {

    cerr << "Warning: Could not show desktop Dialog. Is "
            "'kdialog' installed?"
         << endl;
  }
}
void send_notification(const string &msg) {
  string command = "notify-send 'Timer Alert!' '" + msg +
                   "' -u critical -i 'dialog-information'";
  int res = system(command.c_str());
  if (res != 0) {
    cerr << "Warning: Could not show desktop notification. Is "
            "'notify-send' installed?"
         << endl;
  }
}

void kv_set(const string &key, const std::string &value, StringMap &data_store,
            std::mutex &data_store_mutex) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  data_store[key] = value;
}

bool kv_del(const string &key, StringMap &data_store,
            std::mutex &data_store_mutex) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  return data_store.erase(key) > 0;
}

ListOfStringPairOrNothing kv_getall(StringMap &data_store,
                                    std::mutex &data_store_mutex) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  std::vector<std::pair<std::string, std::string>> pairs;

  for (const auto &[key, value] : data_store) {
    pairs.emplace_back(key, value);
  }
  return pairs;
}

void send_response(int client_fd, const std::string &resp) {
  send(client_fd, resp.c_str(), resp.length(), 0);
}

std::optional<std::string> kv_get(const std::string &key, StringMap &data_store,
                                  std::mutex &data_store_mutex) {
  std::lock_guard<std::mutex> guard(data_store_mutex);
  auto it = data_store.find(key);
  if (it != data_store.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<std::string> tokenize(const std::string &str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;

  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}
} // namespace utils
