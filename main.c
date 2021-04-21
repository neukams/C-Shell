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

void printf_custom(char *s)
{
  printf("%s", s);
  fflush(stdout);
}

/* Given the user input, parses it into the command struct.
  returns a command struct
*/
struct command *parse_input(char *input)
{
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
  printf("parsed_input->full text: %s", parsed_input->full_text);
  fflush(stdout);
  
  // --TODO--
  // PARSE INPUT INTO DIFFERENT COMPONENTS.


  return parsed_input;
}

/* This function kicks off the program.
- Displays the shell to the user
- Get command input
*/
void small_shell()
{
  // store user commands here
  char *input = calloc(MAX_COMMAND_LEN+1, sizeof(char));
  struct command *user_command;
  
  // get commands/input from user
  printf_custom(": ");
  scanf("%2048[^\n]", input);

  /*
  // NOT NEEDED FOR FINAL PROGRAM. Both of these are True if command is just 'ls'
  if (strstr(commands, "ls")) {
    printf("ls is found in the commands string\n");
  }
  if (strncmp("ls", commands, 2048) == 0) {
    printf("ls is equal to the commands string\n");
  }
  printf("%s\n", commands);
  */

  // parse command. returns command struct
  user_command = parse_input(input);

  
  free(input);
}

/* execute the program
*/
int main(void) {
  small_shell();
  return EXIT_SUCCESS;
}