#include <iostream>
#include <sys/sysinfo.h>

int main() {
  struct sysinfo info;
  double factor = 1024.0 * 1024.0;
  if (sysinfo(&info) != -1) {
    std::cout << "Total RAM: " << info.totalram / factor << " MB\n";
    std::cout << "Free RAM : " << info.freeram / factor << "MB\n";
    std::cout << "Total SWAP : " << info.totalswap / factor << "MB\n";
    std::cout << "Free SWAP : " << info.freeswap / factor << "MB\n";
  }
}
