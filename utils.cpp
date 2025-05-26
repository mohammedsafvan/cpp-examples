#include "utils.h"
#include <cstdlib>
#include <iostream>

using std::cerr;
using std::endl;

namespace utils {

void play_sound() {
  int res = system("paplay /usr/share/sounds/freedesktop/stereo/bell.oga &");
  if (res != 0) {
    cerr << "Warning: Could not play sound alarm. Is 'paplay' installed "
            "and a sound file available?"
         << endl;
  }
}
void send_notification(const std::string &msg) {
  std::string command = "notify-send 'Timer Alert!' '" + msg +
                        "' -u critical -i 'dialog-information'";

  int res = system(command.c_str());
  if (res != 0) {
    std::cerr << "Warning: Could not show desktop notification. Is "
                 "'notify-send' installed?"
              << std::endl;
  }
}
} // namespace utils
