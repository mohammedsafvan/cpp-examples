#include "utils.h"
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using std::cerr;
using std::endl;

const std::string PID_FILE_PATH = "/tmp/term_timer.pid";
volatile sig_atomic_t g_terminate_flag = 0;

/*
 * The format of start is changed, Now the user can provide `s` for seconds,`m`
 * for minutes and `h` for hours. Example: 2h3m1s
 */

void handle_signal(int sig_number);
void daemonize();
void run_timer_daemon_task(int duration_seconds);
bool is_daemon_running();
void create_pid_file();
double parse_duration_seconds(const std::string &);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <command> [args...]" << endl;
    cerr << "Commands:" << endl;
    cerr << "  start <duration_seconds>" << endl;
    cerr << "  stop" << endl;
    cerr << "  status" << endl;
    return 1;
  }
  std::string command = argv[1];

  if (command == "start") {
    if (argc != 3) {
      cerr << "Usage: " << argv[0] << " start <duration_seconds>" << endl;
      return 1;
    }

    int duration_in_seconds;

    try {
      duration_in_seconds = parse_duration_seconds(argv[2]);
    } catch (const std::invalid_argument &ia) {
      cerr << "Error: Invalid duration '" << argv[2] << "'. " << ia.what()
           << endl;
      return 1;
    }
    if (duration_in_seconds <= 0) {
      cerr << "Error: Duration must be a positive number of seconds." << endl;
      return 1;
    }

    // We need to check whether the daemon is running or not
    if (is_daemon_running()) {
      cerr << "Daemon is already running or PID file " << PID_FILE_PATH
           << " is stale but points to an active process." << endl;
      cerr << "Use '" << argv[0] << " status' and '" << argv[0] << " stop'."
           << endl;
      return 1;
    }
    std::cout << "Starting the timer daemon";

    if (signal(SIGINT, handle_signal) == SIG_ERR)
      perror("Cannot set SIGINT handler");

    if (signal(SIGTERM, handle_signal) == SIG_ERR)
      perror("Cannot set SIGTERM handler");

    daemonize();

    // The PID file should be created by the daemonized child process
    create_pid_file();
    run_timer_daemon_task(duration_in_seconds);

  } else if (command == "stop") {
    std::cout << "Stopping timer daemon..." << endl;
    std::ifstream pid_file(PID_FILE_PATH);
    if (!pid_file.is_open()) {
      cerr << "Error: PID file " << PID_FILE_PATH
           << " not found. Daemon not running or PID file missing." << endl;
      return 1;
    }

    pid_t pid;
    pid_file >> pid;
    if (pid_file.fail() || pid <= 0) {
      cerr << "Error: Invalid PID in " << PID_FILE_PATH << "." << endl;
      pid_file.close();
      std::remove(PID_FILE_PATH.c_str());
      return 1;
    }
    pid_file.close();

    // Checking the existance of process; ESRCH(No such process)
    if (kill(pid, 0) == -1 && errno == ESRCH) {
      cerr << "Process with PID " << pid
           << " not found. Removing stale PID file." << endl;
      std::remove(PID_FILE_PATH.c_str());
      return 1;
    }

    if (kill(pid, SIGTERM) == 0) {
      std::cout << "Sent SIGTERM to process " << pid
                << ". Waiting for it to stop..." << endl;
      for (int i = 0; i <= 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!is_daemon_running()) {
          std::cout << "Daemon stopped." << endl;
          return 0;
        }
      }
      cerr << "Daemon (PID " << pid
           << ") did not stop gracefully after 5 seconds." << endl;
      cerr << "You might need to use 'kill -9 " << pid << "' if it's stuck."
           << endl;
      return 1;
    } else {
      perror(
          ("Failed to send SIGTERM to process " + std::to_string(pid)).c_str());
      if (errno == ESRCH) {
        std::cout << "Process " << pid
                  << " not found (might have already exited)." << std::endl;
        std::remove(PID_FILE_PATH.c_str());
      }
      return 1;
    }
  } else if (command == "status") {
    if (is_daemon_running()) {
      std::ifstream pid_file(PID_FILE_PATH);
      pid_t pid;
      if (pid_file.is_open()) {
        pid_file >> pid;
        pid_file.close();
      }
      if (pid > 0) {
        std::cout << "Timer daemon is running (PID: " << pid << ")." << endl;
      } else {
        std::cout << "Timer daemon is running (PID could not be read from "
                     "file, but process check passed)."
                  << endl;
      }
    } else {
      std::cout << "Timer daemon is not running!" << endl;
    }
  } else {
    cerr << "Error: Unknown command '" << command << "'." << endl;
    cerr << "Use 'start', 'stop', or 'status'." << endl;
    return 1;
  }
  return 0;
}

void run_timer_daemon_task(int duration_seconds) {
  for (int i = 0; i < duration_seconds; ++i) {
    if (g_terminate_flag) { // Termination by SIGNAL
      utils::send_notification("Timer stopped Prematurely by SIGNAL");
      std::remove(PID_FILE_PATH.c_str());
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
  std::remove(PID_FILE_PATH.c_str()); // Normal completion
  exit(EXIT_SUCCESS);
}

bool is_daemon_running() {
  std::ifstream pid_file(PID_FILE_PATH);
  if (!pid_file.is_open()) {
    return false;
  }

  pid_t pid;
  pid_file >> pid;

  if (pid_file.fail() || pid <= 0) { // Corrupted PID File
    pid_file.close();
    std::remove(PID_FILE_PATH.c_str());
    return false;
  }
  pid_file.close();
  if (kill(pid, 0) == 0) { // Just checking the process whether the process
                           // exists and the user allowed to send signals
    return true;
  } else {
    if (errno == ESRCH) { // if the signal errorno(provided by cerror) ==
                          // process not exists
      std::remove(PID_FILE_PATH.c_str());
    }
    return false;
  }
}

void create_pid_file() {
  if (is_daemon_running()) {
    cerr << "Error: Daemon seems to be already running (PID file exists and "
            "process active)."
         << endl;
    cerr << "If not, remove " << PID_FILE_PATH << " manually." << endl;
    exit(EXIT_FAILURE);
  }

  std::ofstream pid_file(PID_FILE_PATH, std::ios::out | std::ios::trunc);
  if (!pid_file.is_open()) {
    perror(("Error creating PID File at : " + PID_FILE_PATH).c_str());
    exit(EXIT_FAILURE);
  }

  pid_file << getpid();
  if (pid_file.fail()) {
    pid_file.close();
    perror(("Error writing to PID file: " + PID_FILE_PATH).c_str());
    std::remove(PID_FILE_PATH.c_str());
    exit(EXIT_FAILURE);
  }
  pid_file.close();
}

double parse_duration_seconds(const std::string &duration_str) {
  std::string digit_str = "";
  double total_sec = 0;
  for (const char &c : duration_str) {
    if (std::isdigit(c)) {
      digit_str += c;
    } else {
      if (!digit_str.empty()) {
        char unit = std::tolower(c);
        long long val;
        try {
          val = std::stoll(digit_str);
        } catch (const std::invalid_argument &ia) {
          return -1;
        } catch (const std::out_of_range &oor) {
          return -1;
        }

        if (unit == 'h') {
          total_sec += val * 3600;
        } else if (unit == 'm') {
          total_sec += val * 60;
        } else if (unit == 's') {
          total_sec += val;
        } else {
          return -1;
        }

        digit_str.clear();
      } else {
        std::cout << "char " << c << " don't have any digit to associate with!";
        return -1;
      }
    }
  }
  return (total_sec > 0 && total_sec <= 2147483647)
             ? static_cast<int>(total_sec)
             : -1;
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

void handle_signal(int sig_number) {
  if (sig_number == SIGINT || sig_number == SIGTERM) {
    g_terminate_flag = 1;
  }
}
