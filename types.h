#ifndef MY_TYPES
#define MY_TYPES

#include <time.h>

typedef struct command {
  int token_count;
  int status;
  char* name;
  char** args;
  struct tm time;
} Command;

typedef struct {
  int back; /* first available slot */
  int list_size;
  Command** list;
} CommandLog;

typedef struct env {
  char* name;
  char* value;
  struct env* next;
} EnvVar;

typedef struct {
  char* begin;
  char* end;
} Theme;

#endif
