#include <fstream>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>

int main() {
  unsigned int num_cores = std::thread::hardware_concurrency();
  std::cout << "Number of cores are " << num_cores << std::endl;

  std::ifstream cpu_info_file("/proc/cpuinfo");
  std::string line;
  int processors = 0;
  std::string model_name;
  std::set<std::pair<int, int>> physical_cores;

  if (!cpu_info_file.is_open()) {
    std::cerr << "Couldn't open cpuinfo file" << std::endl;
    return 1;
  }

  int current_physical_id = -1;
  int current_core_id = -1;

  while (std::getline(cpu_info_file, line)) {
    // if the line start with processor; which means the processor
    // specification begins
    if (line.find("processor") == 0) {
      processors++;
    }
    if (line.find("model name") == 0) {
      // We just need any of the model as all processor core model name come
      // under one processor
      if (model_name.empty()) {
        model_name = line.substr(
            line.find(":") +
            2); // model name      : AMD Ryzen 5 5500U with Radeon Graphics
      }
    }

    if (line.find("physical id") == 0) {
      std::stringstream pysical_id(line.substr(line.find(":") + 2));
      pysical_id >> current_physical_id;
    }
    if (line.find("core id") == 0) {
      std::stringstream core_id(line.substr(line.find(":") + 2));
      core_id >> current_core_id;

      if (current_physical_id != -1 && current_core_id != -1) {
        physical_cores.insert({current_physical_id, current_core_id});
      }
    }
  }
  cpu_info_file.close();
  std::cout << "CPU Information:" << std::endl;
  std::cout << "  Model name is : " << model_name << std::endl;
  std::cout << "  Logical processors (threads) is : " << processors
            << std::endl;
  std::cout << "  Physical cores is : " << physical_cores.size() << std::endl;

  for (auto pair : physical_cores) {
    std::cout << "Physical core id : " << pair.first << "\t"
              << "Core id : " << pair.second << std::endl;
  }
}
