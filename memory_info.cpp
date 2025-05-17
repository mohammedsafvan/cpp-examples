#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

struct MemoryStats {
  unsigned long total_memory, freed_memory, available_memory, cached_memory,
      buffered_memory, total_swap, freed_swap, cached_swap;
};

unsigned long ReadBytesFromLine(std::string &line) {
  std::string label, unit;
  unsigned long value;
  std::istringstream iss(line);
  iss >> label >> value >> unit;
  return value;
}

MemoryStats ReadMemoryInfo() {
  std::ifstream memfile("/proc/meminfo");
  std::string line;
  MemoryStats output;

  while (std::getline(memfile, line)) {
    if (line.find("MemTotal:") == 0) {
      output.total_memory = ReadBytesFromLine(line);
    }
    if (line.find("MemFree:") == 0) {
      output.freed_memory = ReadBytesFromLine(line);
    }
    if (line.find("MemAvailable:") == 0) {
      output.available_memory = ReadBytesFromLine(line);
    }
    if (line.find("Cached:") == 0) {
      output.cached_memory = ReadBytesFromLine(line);
    }
    if (line.find("Buffers:") == 0) {
      output.buffered_memory = ReadBytesFromLine(line);
    }
    if (line.find("SwapTotal:") == 0) {
      output.total_swap = ReadBytesFromLine(line);
    }
    if (line.find("SwapFree:") == 0) {
      output.freed_swap = ReadBytesFromLine(line);
    }
    if (line.find("SwapCached:") == 0) {
      output.cached_swap = ReadBytesFromLine(line);
    }
  }
  return output;
}

int main() {
  MemoryStats stats = ReadMemoryInfo();
  std::cout << "Memory Statistics:\n";
  std::cout << "  Total Memory     : " << stats.total_memory << " kB\n";
  std::cout << "  Free Memory      : " << stats.freed_memory << " kB\n";
  std::cout << "  Available Memory : " << stats.available_memory << " kB\n";
  std::cout << "  Cached Memory    : " << stats.cached_memory << " kB\n";
  std::cout << "  Buffered Memory  : " << stats.buffered_memory << " kB\n";
  std::cout << "  Total Swap       : " << stats.total_swap << " kB\n";
  std::cout << "  Free Swap        : " << stats.freed_swap << " kB\n";
  std::cout << "  Cached Swap      : " << stats.cached_swap << " kB\n";
}
