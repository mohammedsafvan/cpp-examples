#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;

struct SearchResult {
  int line_number;
  std::string line_content;
  std::string file_path;
};

std::mutex g_all_results_mutex;
std::vector<SearchResult> g_all_results;

void seach_in_files(const std::string &keyword, const std::string &file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    cerr << "Error: Could not open file " << file_path << endl;
    return;
  }

  std::vector<SearchResult> local_thread_results;
  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    line_number++;
    if (line.find(keyword) != std::string::npos) {
      local_thread_results.push_back({line_number, line, file_path});
    }
  }

  if (!local_thread_results.empty()) {
    std::lock_guard<std::mutex> guard(g_all_results_mutex);
    g_all_results.insert(g_all_results.end(), local_thread_results.begin(),
                         local_thread_results.end());
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " <keyword> <file1> [<file2> ...]" << endl;
    return 1;
  }
  std::string keyword = argv[1];
  std::vector<std::thread> threads;

  for (int i = 2; i < argc; i++) {
    std::string file_path = argv[i];
    cout << "Creating thread to search in " << file_path << "..." << endl;

    threads.emplace_back(seach_in_files, keyword, file_path);
  }

  cout << "Waiting for threads to finish..." << endl;
  for (std::thread &t : threads) {
    t.join();
  }

  cout << "\n--- Search Results ---" << endl;
  if (g_all_results.empty()) {
    cout << "Keyword '" << keyword << "' not found." << endl;
  } else {
    for (const SearchResult &result : g_all_results) {
      cout << "[" << result.file_path << ":" << result.line_number << "] "
           << result.line_content << endl;
    }
  }
  return 0;
}
