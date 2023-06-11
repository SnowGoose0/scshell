#ifndef MY_CSHELL
#define MY_CSHELL

#define COLOR_RED                 "\e[0;31m"
#define COLOR_GRN                 "\e[0;32m"
#define COLOR_BLU                 "\e[0;34m"
#define COLOR_NON                 "\e[0m"

#define PROMPT_PREFIX             "%scshell$ %s"
#define FILE_DELIMITER            "\n"
#define COMMAND_DELIMITER         " \n\t\r"
#define COMMAND_MAX_TOKENS        4096

#define ENV_VAR_PREFIX            '$'

#define MODE_I                    0x0
#define MODE_S                    0x1
#define MODE_E                    0x2

#endif
