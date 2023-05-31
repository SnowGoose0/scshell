#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"

#include <sys/types.h>
#include <sys/wait.h>

void handle_exit(Theme* t) {
  printf("%sBye!\n%s", t->begin, t->end);
  exit(EXIT_SUCCESS);
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
  
  printf("%stheme not supported\n%s", t->begin, t->end);
  return 1;
}

int handle_log(Command* c, Theme* t) {
  return 0;
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
  int p_status;
  int c_status;
  char** exec_args;
  
  pid_t pid = fork();

  exec_args = c->args;
  
  if (pid == 0) {
    c_status = execvp(c->name, exec_args);
    
    if (c_status < 0) {
      printf("%s%s: command not found\n%s", t->begin, c->name, t->end);
      exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
    
  } else {
    pid_t cid = wait(&p_status);

    if (cid < 0) return -1;
  }

  return c_status;
}
