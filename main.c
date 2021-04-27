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
#include <stdbool.h>

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
    char *arguments[MAX_ARGUMENTS];
    char *redirect_in;
    char *redirect_out;
    bool background_process;
    char *freetext;
};

/* 
  Function printf_custom()
  ---------------------------------
  description:
    print a string to the console, with or without a newline.
    immediately flushes stdin and stdout
  arg s: char pointer, the string to output to the console
  arg newline: 1 to print a newline after the string is printed to the console, 0 if not
  returns: void
*/
void printf_custom(char *s, int newline)
{
  if (newline == 0) {
    printf("%s", s);
  }

  else if (newline == 1) {
    printf("%s\n", s);
  } 

  fflush(stdin);
  fflush(stdout);
  return;
}

/* 
  Function variable_expansion()
  ---------------------------------
  description:
    expands all instances of $$ into the process ID of this small shell program.
    (pass by reference) edits to $$ made at memory location pointed to by arg input
  arg input: char pointer, the full user input from stdin
  returns: void
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

  strncpy(first_half, input, first_half_length);
  strncpy(second_half, &input[first_half_length+2], second_half_length+1);
  sprintf(input, "%s%d%s", first_half, getpid(), second_half);
  variable_expansion(input);
}

/* 
  Function copytoken()
  ---------------------------------
  description:
    given a token from the user input, place it in the place_here pointer.
    done by reference
    does not copy over any \n that may appear at the end of the token
    assumes each pointer has already had memory allocated for it
    assumes that if the \n character appears, it is at the end of the token
  arg place_here: a char pointer to place the token in
  arg token: char pointer token from the user's input
  returns: void
*/
void copytoken(char *place_here, char *token)
{
  char *newline = strstr(token, "\n");

  if (newline == NULL) {
    strcpy(place_here, token);
  }
  else if (strcmp(newline, "\n") == 0){
    strncpy(place_here, token, strlen(token) - 1);
  }
  printf("placed_here: %s\n", place_here);
  return;
}

/* 
  Function parse_input()
  ---------------------------------
  description:
    parses the user input into a command struct
  arg input: char pointer, the full user input from the console
  returns: a command struct containing the parsed user input
*/
struct command *parse_input(char *input)
{
  // For use with strtok_r to tokenize input
  char *saveptr;
  char *token;

  // allocate memory / initialize
  struct command *parsed_input = malloc(sizeof(struct command));
  parsed_input->full_text = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->command = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->freetext = calloc(strlen(input) + 1, sizeof(char));
  parsed_input->background_process = false;

  strcpy(parsed_input->full_text, input);

  // return if input is simply the newline character
  if (strcmp(input, "\n") == 0) {
    return parsed_input;
  }
  
  // place arguments in the arguments array
  int i;
  for (i=0;i<MAX_ARGUMENTS;i++){
      parsed_input->arguments[i] = NULL;
  }  

  int j;
  for (j=0;j<MAX_ARGUMENTS; j++){
    
    // get the correct token (the next string in the user input)
    if(j < 1){
      token = strtok_r(input, " ", &saveptr);
      copytoken(parsed_input->command, token);
    }
    else {
      token = strtok_r(NULL, " ", &saveptr);
    }

    // if we have reached the final user input string
    if(token == NULL){
      if (j > 0) {
        if (strcmp(parsed_input->arguments[j-1], "&") == 0) {
          parsed_input->background_process = true;          
        }
      }

      break;
    }

    parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
    copytoken(parsed_input->arguments[j], token);
  }

  // handle echo command
  char *saveptr2;
  char *token2;
  char *ret;
  char *temp_full_input = calloc(strlen(input)+1, sizeof(char));
  
  strcpy(temp_full_input, parsed_input->full_text);
  ret = strstr(temp_full_input, "echo ");
  
  if (ret != NULL){
      if (ret - temp_full_input == 0) {
      token2 = strtok_r(temp_full_input, " ", &saveptr2);
      copytoken(parsed_input->freetext, saveptr2);
    }
  }

  return parsed_input;
}

/* 
  Function is_comment()
  ---------------------------------
  description:
    returns 1 if the input is a comment line, 0 otherwise
  arg parsed_input: command struct, the parsed input from the user
  returns: int
*/
int is_comment(struct command *parsed_input)
{
  char *saveptr;
  char *token;
  char *ret;
  int is_comment;
  char *temp_full_input = calloc(MAX_INPUT+1, sizeof(char));

  strcpy(temp_full_input, parsed_input->full_text);  
  ret = strstr(temp_full_input, "#");
  
  if (ret != NULL){
    if (ret - temp_full_input == 0) {
      is_comment = 1;
    }
    else {
      is_comment = 0;
    }
  }

  free(temp_full_input);
  return is_comment;
}

/* 
  Function execute_cd()
  ---------------------------------
  description:
    executes the cd command with up to one argument
    cd -> changes current working directory to the HOME directory
    cd [arg1] -> changes current working directory specified in [arg1]
      if [arg1] is not a valid directory, error message is printed.
  arg parsed_input: command struct, the parsed input from the user
  returns: void
*/
void execute_cd(struct command *parsed_input)
{
  char *dir;
  char *currdir = calloc(500, sizeof(char));
 
  // get directory to change to
  if (strcmp(parsed_input->full_text, "cd\n") == 0) {
    dir = getenv("HOME");
  }
  else if (parsed_input->arguments[1] != NULL) {
    strcpy(dir, parsed_input->arguments[1]);
  }
  else{
    printf("invalid directory\n");
  }
 
  // change directory
  chdir(dir);
  return;
}

/* 
  Function execute()
  ---------------------------------
  description:
    executes the command entered by the user
  arg parsed_input: command struct, the parsed input from the user
  returns: ?
*/
int execute(struct command *parsed_input)
{
  /* debugging
  printf_custom("parsed_input->full_text:");
  printf_custom(parsed_input->full_text);
  printf("%d", strcmp(parsed_input->full_text, ""));
  printf_custom("\n");
  */

  // handler for comment lines
  if (is_comment(parsed_input) == 1) {
    return 1;
  }

  // handler for empty lines
  else if (strcmp(parsed_input->full_text, "\n") == 0) {
    return 1;
  }

  // hanlder for exit command
  else if (strcmp(parsed_input->full_text, "exit\n") == 0) {
    // TODO: CREATE FUNCTION TO HANDLE EXIT FUNCTIONALITY
    return 0;
  }

  // handler for cd command
  else if (strcmp(parsed_input->command, "cd") == 0){
    execute_cd(parsed_input);
  }
  
}

/* 
  Function execute()
  ---------------------------------
  description:
    this function is called by main to start the shell program.
    1. get input from user
    2. parse input
    3. execute commands
  return: void
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
    user_command = parse_input(input);
    continue_ = execute(user_command);

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