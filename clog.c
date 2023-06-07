#include <stdio.h>
#include <stdlib.h>

#include "clog.h"

CommandLog* create_log() {
  CommandLog* new_log = (CommandLog*) malloc(sizeof(CommandLog));
  new_log->back = 0;
  new_log->list_size = DEFAULT_LOG_SIZE;
  new_log->list = (Command**) malloc(DEFAULT_LOG_SIZE * sizeof(Command*));

  return new_log;
}

void insert_log(CommandLog* log, Command* c) {
  if (log->back == log->list_size) {
    int new_size = DEFAULT_LOG_SIZE + log->list_size;
    log->list = (Command**) realloc(log->list, new_size);
    log->list_size = new_size;
  }
  
  log->list[log->back++] = c;
}

void free_log(CommandLog* log) {
  for (int i = 0; i < log->back; ++i) {
    for (int j = 0; j < log->list[i]->token_count; ++j) {
      free(log->list[i]->args[j]);
    }

    free(log->list[i]->cmd_raw);
    free(log->list[i]->args);
    free(log->list[i]);
  }
  
  free(log->list);
  free(log);
}
