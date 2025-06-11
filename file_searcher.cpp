#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

struct SearchResult {
  int line_number;
  string line_content;
  string file_path;
};

std::mutex g_all_results_mutex;
std::vector<SearchResult> g_all_results;

void Print(const std::vector<SearchResult> &result) {
  for (const SearchResult &res : result) {
    cout << "[" << res.file_path << ":" << res.line_number << "] "
         << res.line_content << endl;
  }
}

void seach_in_files(const string &keyword, const string &file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    cerr << "Error: Could not open file " << file_path << endl;
    return;
  }

  cout << "Checking " << std::this_thread::get_id() << endl;
  std::vector<SearchResult> local_thread_results;
  string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    line_number++;
    if (line.find(keyword) != string::npos) {
      cout << "found one : " << file_path << " " << line_number << " " << line
           << endl;
      local_thread_results.push_back({line_number, line, file_path});
    }
  }

  if (!local_thread_results.empty()) {
    cout << "Local thread is not empty" << endl;
    std::lock_guard<std::mutex> guard(g_all_results_mutex);
    g_all_results.insert(g_all_results.end(), local_thread_results.begin(),
                         local_thread_results.end());
  }
}

class ThreadPool {
public:
  ThreadPool(const size_t &num_threads, const string &keyword) : m_stop(false) {
    auto worker = [this, keyword]() {
      while (true) {
        string file_path;
        {
          std::unique_lock<std::mutex> lock(this->m_queue_mutex);

          // wait until Shutdown signal or work present in the queue
          this->m_condition.wait(lock, [this] {
            return this->m_stop || !this->m_tasks_queue.empty();
          });

          // Shutdown check
          if (this->m_stop && this->m_tasks_queue.empty()) {
            return;
          }

          file_path = this->m_tasks_queue.front();
          this->m_tasks_queue.pop();
        } // Lock is released here
        seach_in_files(keyword, file_path);
      }
    };
    // creating worker Objects and adding to the m_workers;
    for (size_t i = 0; i < num_threads; i++) {
      m_workers.emplace_back(worker);
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);
      m_stop = true;
    }
    m_condition.notify_all();

    for (std::thread &worker : m_workers) {
      worker.join();
    }
  }

  void enqueue(const string &file_path) {
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex); // Locking
      m_tasks_queue.push(file_path);
    } // Lock released
    m_condition.notify_one();
  }

private:
  std::queue<string> m_tasks_queue;
  std::vector<std::thread> m_workers;

  // Synchronization primitives
  std::mutex m_queue_mutex;
  std::condition_variable m_condition;
  bool m_stop;
};

int main(int argc, char *argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " <keyword> <file1> [<file2> ...]" << endl;
    return 1;
  }
  string keyword = argv[1];

  const size_t num_threads = std::max(2u, std::thread::hardware_concurrency());
  cout << "Using a thread pool with " << num_threads << " threads." << endl;

  {
    ThreadPool pool(num_threads, keyword);

    for (int i = 2; i < argc; i++) {
      pool.enqueue(argv[i]);
    }
    cout << "All tasks enqueued. Waiting for workers to finish..." << endl;
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
