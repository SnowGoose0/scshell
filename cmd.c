#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "cmd.h"
#include "clog.h"
#include "types.h"

#include <sys/types.h>
#include <sys/wait.h>

#define TMP_DEFAULT 10

char* const WEEKDAYS_MAP[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
char* const MONTHS_MAP[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int handle_exit(Command* c, Theme* t, EnvVar* v, CommandLog* log, char* fb) {
  /*
  if (c->token_count > 1) {
    printf("%sError: Too many arguments\n%s", t->begin, t->end);
    return COMMAND_FAILURE;
  } */

  printf("%sBye!\n%s", t->begin, t->end);

  free(t);
  free_log(log);
  
  if (fb != NULL) free(fb);

  while (v != NULL) {
    EnvVar* tmp = v->next;
    free(v->name);
    free(v->value);
    free(v);
    v = tmp;
  }

  free(c->cmd_raw);
  free(c->args[0]);
  free(c->args);
  free(c);
  
  exit(EXIT_SUCCESS);
}

int handle_log(Command* c, CommandLog* log, Theme* t) {

  if (c->token_count > 1) {
    printf("%sError: Too many arguments\n%s", t->begin, t->end);
    return COMMAND_FAILURE;
  }
  
  for (int i = 0; i < log->back; ++i) {
    Command* c = log->list[i];
    char* command_name = c->name;
    int status = c->status;

    char* wd = WEEKDAYS_MAP[c->time.tm_wday];
    char* m = MONTHS_MAP[c->time.tm_mon];
    int d = c->time.tm_mday;
    int hr = c->time.tm_hour;
    int mn = c->time.tm_min;
    int s = c->time.tm_sec;
    int yr = c->time.tm_year + 1900;

    printf("%s%s %s %d %d:%d:%d %d%s\n", t->begin, wd, m, d, hr, mn, s, yr, t->end);
    printf(" %s%s %d%s\n", t->begin, command_name, status, t->end);
  }

  return COMMAND_SUCCESS;
}

short check_env_var(char** c, EnvVar* v, Theme* t) {
  char** toks = c;

  while (*toks != 0) {
    EnvVar* tmp_v = v;
    
    if ((*toks)[0] == '$' && strlen(*toks) > 1) {
      if (v == NULL) goto NOT_FOUND;
      
      while (tmp_v) {
	if (!strcmp(*toks, tmp_v->name)) return 1;
	tmp_v = tmp_v->next;
      }

      goto NOT_FOUND;
    }
    
    toks++;
  }

  return 1;

NOT_FOUND:
  printf("%sError: No Environment Variable %s found.\n%s", t->begin, *toks, t->end);
  return 0;
}

int handle_print(Command* c, EnvVar* v, Theme* t) {
  char** content = c->args + 1;

  if (*content == 0) {
    printf("%sError: not enough arguments\n%s", t->begin, t->end);
    return COMMAND_FAILURE;
  }

  if (!check_env_var(content, v, t)) return COMMAND_FAILURE;

  while (*content != 0) {
    char* print_item = *content;   
    printf("%s%s %s", t->begin, print_item, t->end);
    
    ++content;
  }
  
  printf("\n");

  return COMMAND_SUCCESS;
}

int handle_theme(Command* c, EnvVar* v, Theme* t) {

  if (c->token_count > 2) {
    printf("%sError: Too many arguments\n%s", t->begin, t->end);
    return COMMAND_FAILURE;
  } else if (c->token_count == 1) {
    goto BAD_THEME;
  }

  char** content = c->args + 1;
  if (!check_env_var(content, v, t)) return COMMAND_FAILURE;

  char* color = c->args[1];

  if (!strcmp(color, "red")) {
    t->begin = COLOR_RED;
    return COMMAND_SUCCESS;
    
  } else if (!strcmp(color, "blue")) {
    t->begin = COLOR_BLU;
    return COMMAND_SUCCESS;
    
  } else if (!strcmp(color, "green")) {
    t->begin = COLOR_GRN;
    return COMMAND_SUCCESS;
  }
  
BAD_THEME:
  printf("%sunsupported theme\n%s", t->begin, t->end);
  return COMMAND_FAILURE;
}

EnvVar* handle_env_var(Command* c, EnvVar* v, Theme* t) {
  char* var = c->name;
  char* val = c->name;
  int valid_var_name = 1;
  EnvVar* head = v;
  EnvVar* prev = NULL;
  
  if (!strstr(var, "=")) {
    printf("%sVariable value expected\n%s", t->begin, t->end);
    return NULL;
  }

  if (c->token_count > 1) {
    printf("%sError: Too many arguments\n%s", t->begin, t->end);
    return NULL;
  }

  while (*val != '=') {
    if (*val != '$' && *val != '_' && !isalnum(*val)) valid_var_name = 0;
    *val = tolower(*val);
    ++val;
  }

  if (!valid_var_name) {
    return NULL;
  }

  
  *val = 0;
  val++;

  if (*val == 0) {
    printf("%sVariable value expected\n%s", t->begin, t->end);
    return NULL;
  }

  char* var_copy = (char*) calloc(strlen(var) + 1, sizeof(char));
  char* val_copy = (char*) calloc(strlen(val) + 1, sizeof(char));
  strcpy(var_copy, var);
  strcpy(val_copy, val);

  *(val - 1) = '=';

  while (v != NULL) {
    if (!strcmp(v->name, var_copy)) {
      free(var_copy);
      free(v->value);
      v->value = val_copy;

      return head;
    }

    prev = v;
    v = v->next;
  }
  
  EnvVar* new_var = (EnvVar*) malloc(sizeof(EnvVar));
  new_var->name = var_copy;
  new_var->value = val_copy;
  new_var->next = NULL;

  if (prev != NULL) {
    prev->next = new_var;
    return head;
  }

  return new_var;
}

int handle_external(Command* c, Theme* t) {
  int pipef[2];
  int c_status;
  char** exec_args;

  if (pipe(pipef) == -1) return COMMAND_FAILURE;
  pid_t pid = fork();

  exec_args = c->args;
  
  if (pid == 0) {
    dup2(pipef[1], STDOUT_FILENO);
    dup2(pipef[1], STDERR_FILENO);
    c_status = execvp(c->name, exec_args);
    
    if (c_status < 0) {
      printf("%sMissing keyword or command, or permission problem\n%s", t->begin, t->end);
      exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
    
  } else {
    wait(&c_status);
    close(pipef[1]);

    char tmp_buffer[TMP_DEFAULT];
    char* read_buffer = (char*) malloc(sizeof(char) * TMP_DEFAULT);
    int read_buffer_size = TMP_DEFAULT;
    int total_bytes_read = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(pipef[0], tmp_buffer, TMP_DEFAULT - 1)) > 0) {
      total_bytes_read += bytes_read;

      if (total_bytes_read > read_buffer_size) {
	read_buffer = (char*) realloc(read_buffer, total_bytes_read + TMP_DEFAULT);
	read_buffer_size += TMP_DEFAULT;
      }

      memcpy(read_buffer + total_bytes_read - bytes_read, tmp_buffer, bytes_read);
      read_buffer[total_bytes_read] = 0;
    }

    close(pipef[0]);
    printf("%s%s%s", t->begin, read_buffer, t->end);

    free(read_buffer);
  }

  return c_status - 255 > 0 ? COMMAND_FAILURE : COMMAND_SUCCESS;
}
