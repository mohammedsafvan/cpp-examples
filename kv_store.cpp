#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

// Hashmap {key : value}
std::unordered_map<std::string, std::string> data_store;

// Accepts address of string mentioned so not copying the data
// And const means we are not changing the key address
void set_value(const std::string &key, const std::string &value) {
  data_store[key] = value;
  std::cout << "SET OK!" << std::endl;
}

// The return value is not ensured. hence, used optional.
std::optional<std::string> get_value(const std::string &key) {
  auto it = data_store.find(key);
  // if it == end it means not found
  if (it != data_store.end()) {
    // it->first refers to `key`
    // it->second refers to `value`
    return it->second;
  }
  return std::nullopt;
}

void delete_value(const std::string &key) {
  if (data_store.erase(key)) {
    std::cout << "DELETED KEY : " << std::endl;
  } else {
    std::cout << "Key not found!" << std::endl;
  }
}
int main() {
  set_value("name", "mini-redis");
  set_value("version", "0.0.1");

  std::cout << *get_value("name") << std::endl;
  set_value("name", "New-mini-redis");
  std::cout << "After updation " << *get_value("name") << std::endl;
  std::cout << "before Version : " << *get_value("version") << std::endl;
  delete_value("version");
  std::optional<std::string> version_val = get_value("version");
  if (version_val) {
    std::cout << "After Version : " << *get_value("version") << std::endl;
  } else {

    std::cout << "not present" << std::endl;
  }
}
