#ifndef _LOG_H_
#define _LOG_H_

/* Asciiverse: a session log, session.log in the save folder (SAVE_DIR), begun
 * afresh each time the game starts.  log_info() lines are always written;
 * log_debug() lines only when EDIGOTIA_LOG or RUST_LOG is set to "debug" (the
 * catalog's convention).
 */

void log_start();
void log_info (const char* mes, ...);
void log_debug(const char* mes, ...);

#endif
