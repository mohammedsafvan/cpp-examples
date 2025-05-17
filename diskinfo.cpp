#include <iostream>
#include <sys/statvfs.h>

void PrintDiskUsage(const char *path) {
  struct statvfs stat;

  if (statvfs(path, &stat) != 0) {
    perror("statvfs failed");
    return;
  }

  unsigned long block_size = stat.f_frsize;
  unsigned long total_blocks = stat.f_blocks;
  unsigned long free_blocks = stat.f_bfree;
  unsigned long available_blocks = stat.f_bavail;

  unsigned long total_size = total_blocks * block_size;
  unsigned long free_size = free_blocks * block_size;
  unsigned long available_size = available_blocks * block_size;

  std::cout << "Filesystem stats for: " << path << "\n";
  std::cout << "block_size : " << block_size << "\n";
  std::cout << "  Total Size     : " << total_size / (1024 * 1024 * 1024)
            << " GB\n";
  std::cout << "  Free Size      : " << free_size / (1024 * 1024 * 1024)
            << " GB\n";
  std::cout << "  Available Size : " << available_size / (1024 * 1024 * 1024)
            << " GB\n";
}

int main() { PrintDiskUsage("/"); }
