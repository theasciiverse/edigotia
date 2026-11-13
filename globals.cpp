#include "globals.h"
#include "files.h"
#include "help.h"
#include "window.h" // For debugmsg()

Game* GAME;

Help_database* HELP;

std::string SAVE_DIR;

#if !(defined _WIN32 || defined WINDOWS)
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Asciiverse: the folder the program is in, or "" if it can't be found.
static std::string exe_dir()
{
  char buf[PATH_MAX];
#ifdef __APPLE__
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0) {
    return "";
  }
  char real[PATH_MAX];
  if (!realpath(buf, real)) {
    return "";
  }
  std::string p(real);
#else
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) {
    return "";
  }
  buf[n] = 0;
  std::string p(buf);
#endif
  size_t slash = p.rfind('/');
  return (slash == std::string::npos ? "" : p.substr(0, slash));
}

static void make_dirs(const std::string& path)
{
  for (size_t i = 1; i <= path.size(); i++) {
    if (i == path.size() || path[i] == '/') {
      mkdir(path.substr(0, i).c_str(), 0755);
    }
  }
}
#endif

/* Asciiverse: when the game's files (cuss/, motd.txt, art.txt) are beside the
 * program, it reads them from there, wherever it was started, and keeps its
 * saves in the player's own folder: $XDG_STATE_HOME/edigotia (usually
 * ~/.local/state/edigotia), or ~/Library/Application Support/edigotia on
 * macOS. Started from the source folder, it uses save/ there as before.
 */
bool set_directories()
{
  SAVE_DIR = "save/";
#if !(defined _WIN32 || defined WINDOWS)
  std::string dir = exe_dir();
  if (!dir.empty() && directory_exists(dir + "/cuss") &&
      chdir(dir.c_str()) == 0) {
    const char* home = getenv("HOME");
    std::string state;
#ifdef __APPLE__
    if (home) {
      state = std::string(home) + "/Library/Application Support/edigotia";
    }
#else
    const char* xdg = getenv("XDG_STATE_HOME");
    if (xdg && *xdg) {
      state = std::string(xdg) + "/edigotia";
    } else if (home) {
      state = std::string(home) + "/.local/state/edigotia";
    }
#endif
    if (!state.empty()) {
      make_dirs(state);
      SAVE_DIR = state + "/";
    }
  }
#endif
  if (!directory_exists(SAVE_DIR) && !create_directory(SAVE_DIR)) {
    debugmsg("Unable to create directory '%s'.", SAVE_DIR.c_str());
    return false;
  }
  return true;
}
