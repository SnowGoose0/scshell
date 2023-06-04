#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void handle_exit(Theme* t, EnvVar* v, CommandLog* log, char* fb) {
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
  
  exit(EXIT_SUCCESS);
}

int handle_log(Command* c, CommandLog* log, Theme* t) {

  if (c->token_count > 1) return 1;
  
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
    printf("  %s%s %d%s\n", t->begin, command_name, status, t->end);
  }

  return 0;
}

int handle_print(Command* c, EnvVar* v, Theme* t) {
  char** content = c->args + 1;

  if (*content == 0) {
    printf("%serror: not enough arguments\n%s", t->begin, t->end);
    return 1;
  }

  while (*content != 0) {
    char* print_item = *content;
    EnvVar* tmp_v = v;
    
    if ((*content)[0] == '$') {
      while (tmp_v) {
	if (!strcmp(*content, tmp_v->name)) print_item = tmp_v->value;
	tmp_v = tmp_v->next;
      }
    }
      
    printf("%s%s %s", t->begin, print_item, t->end);
    
    ++content;
  }
  
  printf("\n");

  return 0;
}

int handle_theme(Command* c, Theme* t) {
  if (c->token_count != 2) {
    printf("%serror: invalid arguments\n%s", t->begin, t->end);
    return 1;
  }

  char* color = c->args[1];

  if (!strcmp(color, "red")) {
    t->begin = COLOR_RED;
    return 0;
    
  } else if (!strcmp(color, "blue")) {
    t->begin = COLOR_BLU;
    return 0;
    
  } else if (!strcmp(color, "green")) {
    t->begin = COLOR_GRN;
    return 0;
  }
  
  printf("%sunsupported theme\n%s", t->begin, t->end);
  return 1;
}

EnvVar* handle_env_var(Command* c, EnvVar* v, Theme* t) {
  char* var = c->name;
  char* val = c->name;
  EnvVar* head = v;
  EnvVar* prev = NULL;
  
  if (!strstr(var, "=")) {
    printf("%svariable value expected%s", t->begin, t->end);
    return NULL;
  }

  while (*val != '=') ++val;
  *val = 0;
  val++;

  char* var_copy = (char*) calloc(strlen(var) + 1, sizeof(char));
  char* val_copy = (char*) calloc(strlen(val) + 1, sizeof(char));
  strcpy(var_copy, var);
  strcpy(val_copy, val);

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

  if (pipe(pipef) == -1) return 1;
  pid_t pid = fork();

  exec_args = c->args;
  
  if (pid == 0) {
    dup2(pipef[1], STDOUT_FILENO);
    dup2(pipef[1], STDERR_FILENO);
    c_status = execvp(c->name, exec_args);
    
    if (c_status < 0) {
      printf("%s%s: command not found\n%s", t->begin, c->name, t->end);
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
	read_buffer = (char*) realloc(read_buffer, read_buffer_size + TMP_DEFAULT);
	read_buffer_size += TMP_DEFAULT;
      }

      memcpy(read_buffer + total_bytes_read - bytes_read, tmp_buffer, bytes_read);
      read_buffer[total_bytes_read] = 0;
    }

    close(pipef[0]);
    printf("%s%s%s", t->begin, read_buffer, t->end);

    free(read_buffer);
  }

  return c_status - 255 > 0 ? 1 : 0;
}
