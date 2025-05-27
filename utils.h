#ifndef UTIL_H
#define UTIL_H

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using StringMap = std::unordered_map<std::string, std::string>;

namespace utils {
void send_notification(const std::string &msg);
void play_sound();

void kv_set(const std::string &key, const std::string &value,
            StringMap &data_store, std::mutex &data_store_mutex);
bool kv_del(const std::string &key, StringMap &data_store,
            std::mutex &data_store_mutex);

std::optional<std::string> kv_get(const std::string &key, StringMap &data_store,
                                  std::mutex &data_store_mutex);

void send_response(int client_fd, const std::string &resp);

std::vector<std::string> tokenize(const std::string &str, char delimiter = ' ');
} // namespace utils
#endif // !UTIL_H
