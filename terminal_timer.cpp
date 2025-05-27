#include "utils.h"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;

volatile sig_atomic_t g_terminate_flag = 0;

void handle_signal(int sig_number) {
  if (sig_number == SIGINT || sig_number == SIGTERM) {
    g_terminate_flag = 1;
  }
}
/*
 * Now the process needs to have an option for graceful shutdown, Now the user
 * can kill 🗡️ the process with pid gracefully.
 */
void daemonize();
void run_timer_daemon_task(int duration_seconds);

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
  if (signal(SIGINT, handle_signal) == SIG_ERR) {
    perror("Cannot set SIGINT handler");
  }
  if (signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("Cannot set SIGTERM handler");
  }

  daemonize();
  run_timer_daemon_task(duration_in_seconds);
}

void run_timer_daemon_task(int duration_seconds) {
  for (int i = 0; i < duration_seconds; ++i) {
    if (g_terminate_flag) {
      utils::send_notification("Timer stopped Prematurely by SIGNAL");
      exit(EXIT_SUCCESS);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (!g_terminate_flag) {
    std::string alarm_message = "Your " + std::to_string(duration_seconds) +
                                " second timer has finished.";
    utils::send_notification(alarm_message);
    utils::send_dialog(alarm_message);
  }
  exit(EXIT_SUCCESS);
}

void daemonize() {
  pid_t pid;
  pid = fork();

  if (pid < 0) {
    perror("Fork Failed");
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    // this is parent process, Now we don't need the parent process, we only
    // need the child process from now on
    exit(EXIT_SUCCESS);
  }
  if (setsid() < 0) {
    perror("Setsid failed");
    exit(EXIT_FAILURE);
  }
  if (chdir("/") < 0) {
    perror("Can't change dir");
    exit(EXIT_FAILURE);
  }
  umask(0);
  close(STDIN_FILENO);
  int fd_null = open("/dev/null", O_RDWR);
  if (fd_null != -1) {
    dup2(fd_null, STDERR_FILENO);
    dup2(fd_null, STDOUT_FILENO);
    if (fd_null > STDERR_FILENO) {
      close(fd_null);
    }
  } else {
    close(STDERR_FILENO);
    close(STDOUT_FILENO);
  }
}
