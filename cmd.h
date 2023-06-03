#ifndef MY_COMMANDS
#define MY_COMMANDS

#include "types.h"
#include "cshell.h"

void handle_exit(Theme* t, EnvVar* v, CommandLog* log, char* fb);

int handle_log(Command* c, CommandLog* log, Theme* t);

int handle_print(Command* c, EnvVar* v, Theme* t);

int handle_theme(Command* c, Theme* t);

EnvVar* handle_env_var(Command* c, EnvVar* v, Theme* t);

int handle_external(Command* c, Theme* t);

#endif
