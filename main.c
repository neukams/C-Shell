// #include "headers.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>

#define MAX_COMMAND_LEN 2048
#define MAX_ARGUMENTS 512
#define EXIT "exit"
#define CD "cd"
#define STATUS "status"
#define PID_INT_LENGTH 12


struct command
{
    char *full_text;
    char *command;
    char *argument;
    char *redirect;
    char *filename;
    char *background_process;
    char *freetext;
    int seconds;
    struct command *next;
};

void printf_custom(char *s, int newline)
{
  if (newline == 0) {
    printf("%s", s);
    fflush(stdout);
  }

  else if (newline == 1) {
    printf("%s\n", s);
    fflush(stdout);
  }  
}

/* 
  expands the $$ variable into this program's pid
*/
void variable_expansion(char *input)
{
  if (strstr(input, "$$") == NULL){
    return;
  }
  
  int second_half_length = strlen(strstr(input, "$$"));
  int first_half_length = strlen(input) - second_half_length;
  char *first_half = calloc(first_half_length+1, sizeof(char));
  char *second_half = calloc(second_half_length+1, sizeof(char));
  char *delim = "$$";

  strncpy(first_half, input, first_half_length);
  strncpy(second_half, &input[first_half_length+2], second_half_length+1);
  sprintf(input, "%s%d%s", first_half, getpid(), second_half);
  variable_expansion(input);
}

/* 
  Given the user input, parses it into the command struct.
  returns a command struct
*/
struct command *parse_input(char *input)
{
  // For use with strtok_r
  char *saveptr;
  char *token;

  // allocate memory for struct variables
  struct command *parsed_input = malloc(sizeof(struct command));
  parsed_input->full_text = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->command = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->argument = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->redirect = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->filename = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->background_process = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->freetext = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->next = malloc(sizeof(struct command));

  // move data into struct variables
  strcpy(parsed_input->full_text, input);
  //printf_custom(input);
  //printf("parsed_input->full text: %s", parsed_input->full_text);
  //fflush(stdout);
  
  // --TODO--
  // PARSE INPUT INTO DIFFERENT COMPONENTS.
  // I HAVE TAKEN CARE OF 'ECHO' COMMAND, NO OTHERS

  // Parse command
  
  // comment line
  if (strstr(input, "#")) {
    printf_custom("parse_input: this is a comment line, will not do anything", 1);
  }

  // empty line
  else if (strcmp(input, "\n") == 0) {
    printf_custom("parse_input: this is an empty line, will not do anything", 1);
  }

  // echo command
  else if (strstr(input, "echo")) {
    printf_custom("parse_input: echo command", 1);
    token = strtok_r(input, " ", &saveptr);
    printf("parse_input: pringting token: %s\n", token);
    printf("parse_input: pringting saveptr: %s", saveptr);
    strcpy(parsed_input->command, token);
    strcpy(parsed_input->freetext, saveptr);
  }

  return parsed_input;
}

/* 
  execute the command
  argument is the parsed command struct returned from parse_input()
  returns 0 if the command is to exit the program, 1 for all other commands.
*/
int execute_command(struct command *parsed_input)
{
  /* debugging
  printf_custom("parsed_input->full_text:");
  printf_custom(parsed_input->full_text);
  printf("%d", strcmp(parsed_input->full_text, ""));
  printf_custom("\n");
  */

  // handler for blank lines and comment lines (start with #)
  if (strcmp(parsed_input->full_text, "\n") == 0 || strstr(parsed_input->full_text, "#")) {
    printf_custom("execute_command: comment or empty line", 1);
  }

  // hanlder for exit command
  else if (strcmp(parsed_input->full_text, "exit\n") == 0) {
    printf_custom("\n", 0);
    return 0;
  }

  // handler for echo command
  else if (strcmp(parsed_input->command, "echo") == 0) {
    printf_custom(parsed_input->freetext, 1);
  }

  printf_custom("\n", 0);
  return 1;
}

/* 
  This function kicks off the program.
  1. Displays the shell to the user
  2. Get command input
  3. Expand variables
  4. Execute commands
*/
void small_shell()
{
  int continue_ = 1; 
  
  do
  { 
    char *input = calloc(MAX_COMMAND_LEN+1, sizeof(char));
    char *expanded_input;
    struct command *user_command;
    
    printf_custom(": ", 0);
    fgets(input, MAX_COMMAND_LEN+1, stdin);

    variable_expansion(input);
    printf("small_shell: variable expansion result: ");
    printf_custom(input, 1);
    printf_custom("exiting", 1);
    return;

    user_command = parse_input(expanded_input);
    continue_ = execute_command(user_command);

    free(user_command);
    free(input);

  } while (continue_ == 1);
  
}

/* execute the program
*/
int main(void) {
  small_shell();
  return EXIT_SUCCESS;
}