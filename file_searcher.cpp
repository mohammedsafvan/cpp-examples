#include <fstream>
#include <iostream>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;

struct SearchResult {
  int line_number;
  std::string line_content;
  std::string file_path;
};

std::vector<SearchResult> seach_in_files(const std::string &keyword,
                                         const std::string &file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    cerr << "Error: Could not open file " << file_path << endl;
    return {};
  }

  std::vector<SearchResult> results;
  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    line_number++;
    if (line.find(keyword) != std::string::npos) {
      results.push_back({line_number, line, file_path});
    }
  }
  return results;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " <keyword> <file1> [<file2> ...]" << endl;
    return 1;
  }
  std::string keyword = argv[1];
  std::vector<std::string> files_to_search;
  std::vector<SearchResult> all_results;

  for (int i = 2; i < argc; i++)
    files_to_search.push_back(argv[i]);

  for (const auto &file_path : files_to_search) {
    cout << "Searching in " << file_path << "..." << endl;
    const auto search_result = seach_in_files(keyword, file_path);
    all_results.insert(all_results.end(), search_result.begin(),
                       search_result.end());
  }
  cout << "\n--- Search Results ---" << endl;
  if (all_results.empty()) {
    cout << "Keyword '" << keyword << "' not found." << endl;
  } else {
    for (const SearchResult &result : all_results) {
      cout << "[" << result.file_path << ":" << result.line_number << "] "
           << result.line_content << endl;
    }
  }
  return 0;
}
