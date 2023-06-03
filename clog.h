#ifndef CLOG
#define CLOG

#include "types.h"
#define DEFAULT_LOG_SIZE 25

CommandLog* create_log();

void insert_log(CommandLog* log, Command* c);

void display_log(CommandLog* log);

void free_log(CommandLog* log);

#endif
