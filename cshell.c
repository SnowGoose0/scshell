#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "cmd.h"
#include "clog.h"
#include "types.h"
#include "cshell.h"

char* get_var(char* name, EnvVar* v) {
  while (v) {
    if (!strcmp(name, v->name)) {
      return v->value;
    }
    v = v->next;
  }

  return NULL;
}

Command* parse(char* cmd_string, EnvVar* v) {
  struct tm * t;
       
  time_t cur_time;
  time(&cur_time);
  t = localtime(&cur_time);
  
  char* cmd = (char*) calloc(strlen(cmd_string) + 1, sizeof(char));
  strcpy(cmd, cmd_string);
  
  Command* c = (Command*) malloc(sizeof(Command));
  c->cmd_raw = cmd;
  c->token_count = 0;
  c->status = 0;
  c->name = NULL;
  c->args = (char**) calloc(COMMAND_MAX_TOKENS + 1, sizeof(char**));
  c->time = *t;
  
  char* tok = strtok(cmd_string, COMMAND_DELIMITER);

  while (tok != NULL) {
    int tok_len = strlen(tok);
    int tok_index = c->token_count;

    if (tok_len > 1 && *tok == '$' && tok_index != 0) {
      for (int i = 0; i < tok_len; ++i) tok[i] = tolower(tok[i]);
      
      char* var_val = get_var(tok, v);

      if (var_val == NULL) {
	c->status = -1;
      } else {
	tok = var_val;
	tok_len = strlen(var_val);
      }
    }
    
    char* tok_cpy = (char*) calloc(tok_len + 1, sizeof(char));
    strcpy(tok_cpy, tok);
    
    c->args[tok_index] = tok_cpy;
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

/* void print_vars(EnvVar* v) {
  while (v) {
    printf("[%s:%s] ", v->name, v->value);
    v = v->next;
  }

  printf("\n");
} */

int main(int argc, char** argv) {
  char* file_path = NULL;
  int mode; /* 0 = interactive | 1 = read */

  FILE* f;
  char* file_buffer = NULL;
  char* file_pos = NULL;
  EnvVar* var_list = NULL;
  Theme* theme;
  CommandLog* clog;
  char cmd_buffer[COMMAND_MAX_TOKENS];
  clear_buffer(cmd_buffer);

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
      printf("Unable to read script file: %s\n", file_path);
      mode = MODE_I;
      exit(EXIT_FAILURE);
      
    } else {
      fseek(f, 0, SEEK_END);
      int fs = ftell(f);
      fseek(f, 0, SEEK_SET);

      if (fs > 0) {
        file_buffer = (char*) calloc(fs + 1, sizeof(char));
        fread(file_buffer, sizeof(char), fs, f);
        file_pos = file_buffer;
        fclose(f);
      } else {
        strcpy(cmd_buffer, "exit");
        mode = MODE_E;
      }
    }
  }
  
  theme = (Theme*) malloc(sizeof(Theme));
  theme->begin = COLOR_NON;
  theme->end = COLOR_NON;

  clog = create_log();

  if (mode == MODE_S) {
    char* f_tok = strtok(file_buffer, FILE_DELIMITER);
    
    if (f_tok != NULL) strcpy(cmd_buffer, f_tok);
    else {
      strcpy(cmd_buffer, "exit");
    }
  }

  if (mode == MODE_I) {
    printf(PROMPT_PREFIX, COLOR_NON, COLOR_NON);
    fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
  }
  
  for (;;) {
    Command* parsed_cmd = parse(cmd_buffer, var_list);
    char* cmd_name = parsed_cmd->name;

    int status = COMMAND_SUCCESS;    
    if (parsed_cmd->token_count > 0) {
      
      if (!strcmp(cmd_name, "exit")) {
	status = handle_exit(parsed_cmd, theme, var_list, clog, file_buffer);
  
      } else if (!strcmp(cmd_name, "log")) {
	status = handle_log(parsed_cmd, clog, theme);
       
      } else if (!strcmp(cmd_name, "print")) {
        status = handle_print(parsed_cmd, var_list, theme);

      } else if (!strcmp(cmd_name, "theme")) {
	status = handle_theme(parsed_cmd, var_list, theme);

      } else if (*cmd_name == ENV_VAR_PREFIX) {
	EnvVar* tmp_var_list = handle_env_var(parsed_cmd, var_list, theme);

  if (tmp_var_list == NULL) status = COMMAND_FAILURE;
  else var_list = tmp_var_list;

      } else {
	status = handle_external(parsed_cmd, theme);
      }

      parsed_cmd->status = status;
      insert_log(clog, parsed_cmd);
      
    } else {
      free(parsed_cmd->cmd_raw);
      free(parsed_cmd->args[0]);
      free(parsed_cmd->args);
      free(parsed_cmd);
    }

    clear_buffer(cmd_buffer);
    
    if (mode == MODE_S) {
      while (*file_pos++ != 0);
      char* f_tok = strtok(file_pos, FILE_DELIMITER);
      
      if (f_tok != NULL) {
	strcpy(cmd_buffer, f_tok);
      } else {
	strcpy(cmd_buffer, "exit");
      }
    }

    if (mode == MODE_I) {
      printf(PROMPT_PREFIX, theme->begin, theme->end);
      fgets(cmd_buffer, sizeof(cmd_buffer), stdin);      
    }
  }

  exit(EXIT_SUCCESS);
}
