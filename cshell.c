#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "cmd.h"
#include "types.h"
#include "cshell.h"

Command* parse_command_name(char* cmd) {
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

void print_vars(EnvVar* v) {
  while (v) {
    printf("[%s:%s] ", v->name, v->value);
    v = v->next;
  }

  printf("\n");
}

int main(int argc, char** argv) {
  char cmd_buffer[100];
  EnvVar* var_list = NULL;
  
  Theme* theme = (Theme*) malloc(sizeof(Theme));
  theme->begin = COLOR_NON;
  theme->end = COLOR_NON;
  
  printf(PROMPT_PREFIX, COLOR_NON, COLOR_NON);
  fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
  
  for (;;) {
    Command* parsed_cmd = parse_command_name(cmd_buffer);
    
    if (parsed_cmd->token_count > 0) {
      char* cmd_name = parsed_cmd->name;
      int status;
      
      if (!strcmp(cmd_name, "exit")) {
	handle_exit(theme);

      } else if (!strcmp(cmd_name, "log")) {
	printf("log-soon");
       
      } else if (!strcmp(cmd_name, "print")) {
        handle_print(parsed_cmd, var_list, theme);

      } else if (!strcmp(cmd_name, "theme")) {
	handle_theme(parsed_cmd, theme);

      } else if (*cmd_name == ENV_VAR_PREFIX) {
	var_list = handle_env_var(parsed_cmd, var_list, theme);
	print_vars(var_list);

      } else {
	handle_external(parsed_cmd, theme);
      }
    }
    
    printf(PROMPT_PREFIX, theme->begin, theme->end);
    
    free(parsed_cmd);
    fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
  }

  return 0;
}

