#include "utils.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;
/*
 * So in this code base the target is  a timer that can work in the
 * background(in a light weight form) 🔥. By creating a child process 🧒, and
 * running the task on the child process, the running is not dependent on the
 * parent process, so the parent process can be closed without affecting the
 * child(the timer running) process. The child process become an orphan 😢
 * process, those orphans are taken care by 🌟 `init` or `systemd` of linux
 */
void run_timer(int duration_seconds) {
  cout << "Timer Process PID : " << getpid() << " started for "
       << duration_seconds << "seconds" << endl;
  cout << "This is from child process, this process can run in background"
       << endl;

  for (int i = duration_seconds; i > 0; i--) {
    std::cout << "\r[PID " << getpid() << "] Time remaining: " << i << "s... "
              << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  cout << endl;
  cout << "[PID " << getpid() << "] Time's up!" << std::endl;

  std::string msg = "Your " + std::to_string(duration_seconds) +
                    " second timer has finished.";

  utils::send_notification(msg);
  utils::play_sound();

  exit(0);
}

int main(int argc, char *argv[]) {
  int duration_in_seconds;

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <duration_in_seconds>" << endl;
    cerr << "Example: " << argv[0] << " 10" << endl;
    return 1;
  }

  try {
    duration_in_seconds = std::stoi(std::string(argv[1]));
  } catch (const std::invalid_argument &ia) {
    cerr << "Error: Invalid duration format. Please provide a number." << endl;
    cerr << "Caught invalid_argument: " << ia.what() << endl;
    return 1;
  } catch (const std::out_of_range &oor) {
    cerr << "Error: Duration out of range." << endl;
    cerr << "Caught out_of_range: " << oor.what() << endl;
    return 1;
  }

  if (duration_in_seconds <= 0) {
    cerr << "Error: Duration must be a positive number of seconds." << endl;
    return 1;
  }

  pid_t process_id = fork();

  if (process_id < 0) {
    cerr << "Failed to fork Process!" << endl;
    return 1;
  }

  if (process_id > 0) {
    // If the process is parent
    cout << "Process started in the background in child process CHILD PID : "
         << process_id << endl;
    cout << "Parent Process Id : " << getpid() << "; Now exiting" << endl;
    return 0;
  } else {
    // Child Process
    run_timer(duration_in_seconds);
    return 0;
  }
}
