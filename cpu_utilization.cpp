#include <chrono>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>

struct CpuInfo {
  unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;

  unsigned long long Total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
  }
  unsigned long long Idle() const { return idle + iowait; }
};

CpuInfo ReadCpuInfo() {
  std::ifstream file("/proc/stat");
  std::string line;
  CpuInfo info;

  if (std::getline(file, line) && line.find("cpu ") == 0) {
    std::istringstream ss(line);
    std::string string_label;

    ss >> string_label;
    ss >> info.user >> info.nice >> info.system >> info.idle >> info.iowait >>
        info.irq >> info.softirq >> info.steal;
  }
  return info;
}

int main() {
  while (true) {

    CpuInfo info1 = ReadCpuInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CpuInfo info2 = ReadCpuInfo();

    unsigned long long total_diff = info2.Total() - info1.Total();
    unsigned long long idle_diff = info2.Idle() - info1.Idle();

    double cpu_utilization = 100.0 * (total_diff - idle_diff) / total_diff;

    std::cout << "\rCPU cpu_utilization is : " << cpu_utilization << "%"
              << std::flush;
  }
  return 0;
}
