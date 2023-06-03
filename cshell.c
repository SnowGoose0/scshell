#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "cmd.h"
#include "clog.h"
#include "types.h"
#include "cshell.h"

Command* parse(char* cmd_string) {
  char* cmd = (char*) calloc(strlen(cmd_string) + 1, sizeof(char));
  strcpy(cmd, cmd_string);
  
  Command* c = (Command*) malloc(sizeof(Command));
  c->token_count = 0;
  c->status = -1;
  c->name = NULL;
  c->args = (char**) calloc(COMMAND_MAX_TOKENS + 1, sizeof(char**));
  
  char* tok = strtok(cmd, COMMAND_DELIMITER);

  while (tok != NULL) {
    int tok_index = c->token_count;
    c->args[tok_index] = tok;
    c->token_count++;
    
    tok = strtok(NULL, COMMAND_DELIMITER);
  }

  c->name = c->args[0];

  return c;
}

void clear_buffer(char* b) {
  int i = 0;
  while (i++ != COMMAND_MAX_TOKENS) *b = 0;
}

void print_vars(EnvVar* v) {
  while (v) {
    printf("[%s:%s] ", v->name, v->value);
    v = v->next;
  }

  printf("\n");
}

int main(int argc, char** argv) {
  char* file_path = NULL;
  int mode; /* 0 = interactive | 1 = read */

  FILE* f;
  char* file_buffer = NULL;
  char* file_pos = NULL;
  char cmd_buffer[COMMAND_MAX_TOKENS];
  EnvVar* var_list = NULL;
  
  Theme* theme = (Theme*) malloc(sizeof(Theme));
  theme->begin = COLOR_NON;
  theme->end = COLOR_NON;

  CommandLog* clog = create_log();

  switch (argc) {
  case 2:
    mode = MODE_S;
    file_path = argv[1];
    break;
    
  case 1:
    mode = MODE_I;
    break;
    
  default:
    exit(EXIT_FAILURE);
  }

  if (mode == MODE_S) {
    f = fopen(file_path, "r");
    
    if (f == NULL) {
      printf("file not good\n");
      mode = MODE_I;
      
    } else {
      fseek(f, 0, SEEK_END);
      int fs = ftell(f);
      fseek(f, 0, SEEK_SET);

      file_buffer = (char*) calloc(fs + 1, sizeof(char));
      fread(file_buffer, sizeof(char), fs, f);
      file_pos = file_buffer;

      printf("%s\n", file_buffer);
    }

    fclose(f);
  }
  
  printf(PROMPT_PREFIX, COLOR_NON, COLOR_NON);

  if (mode == MODE_S) {
    char* f_tok = strtok(file_buffer, FILE_DELIMITER);
    strcpy(cmd_buffer, f_tok);
    putchar('\n');
    
  } else if (mode == MODE_I) {
    fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
  }
  
  for (;;) {
    Command* parsed_cmd = parse(cmd_buffer);
    int status = 0;
    struct tm * t;
       
    time_t cur_time;
    time(&cur_time);
    t = localtime(&cur_time);

    parsed_cmd->time = *t;
    
    if (parsed_cmd->token_count > 0) {
      char* cmd_name = parsed_cmd->name;
      
      if (!strcmp(cmd_name, "exit")) {
	handle_exit(theme);

      } else if (!strcmp(cmd_name, "log")) {
	status = handle_log(parsed_cmd, clog, theme);
       
      } else if (!strcmp(cmd_name, "print")) {
        status = handle_print(parsed_cmd, var_list, theme);

      } else if (!strcmp(cmd_name, "theme")) {
	status = handle_theme(parsed_cmd, theme);

      } else if (*cmd_name == ENV_VAR_PREFIX) {
	var_list = handle_env_var(parsed_cmd, var_list, theme);
	print_vars(var_list);

      } else {
	status = handle_external(parsed_cmd, theme);
      }
    }

    parsed_cmd->status = status;
    insert_log(clog, parsed_cmd);

    printf(PROMPT_PREFIX, theme->begin, theme->end);

    clear_buffer(cmd_buffer);
    
    if (mode == MODE_S) {
      while (*file_pos++ != 0);
      char* f_tok = strtok(file_pos, FILE_DELIMITER);
      
      if (f_tok != NULL) {
	strcpy(cmd_buffer, f_tok);
	putchar('\n');
      } else {
	mode = MODE_I;
      }
    }

    if (mode == MODE_I) {
      fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
    }
  }

  exit(EXIT_SUCCESS);
}
