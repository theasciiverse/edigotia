#include "log.h"
#include "globals.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>

static FILE* log_file = NULL;
static bool  log_debug_on = false;

static bool env_is_debug(const char* name)
{
  const char* val = getenv(name);
  return (val && strstr(val, "debug"));
}

void log_start()
{
  if (log_file) {
    return;
  }
  log_debug_on = env_is_debug("EDIGOTIA_LOG") || env_is_debug("RUST_LOG");
  std::string filename = SAVE_DIR + "session.log";
  log_file = fopen(filename.c_str(), "w");
  log_info("Edigotia session started%s", log_debug_on ? " (debug log)" : "");
}

static void log_write(const char* level, const char* mes, va_list ap)
{
  if (!log_file) {
    return;
  }
  char stamp[32];
  time_t now = time(NULL);
  strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
  fprintf(log_file, "%s %s ", stamp, level);
  vfprintf(log_file, mes, ap);
  fprintf(log_file, "\n");
  fflush(log_file);
}

void log_info(const char* mes, ...)
{
  va_list ap;
  va_start(ap, mes);
  log_write("INFO ", mes, ap);
  va_end(ap);
}

void log_debug(const char* mes, ...)
{
  if (!log_debug_on) {
    return;
  }
  va_list ap;
  va_start(ap, mes);
  log_write("DEBUG", mes, ap);
  va_end(ap);
}
