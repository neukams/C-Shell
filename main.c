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
    immediately flushes stdin 
  arg s: char pointer, the string to output to the console
  arg newline: 1 to print a newline after the string is printed to the console, 0 if not
  returns: void
*/
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
  
  // initialize arguments to null
  int i;
  for (i=0;i<MAX_ARGUMENTS;i++){
      parsed_input->arguments[i] = NULL;
  }
  
  //printf("\nparsed_input->arguments[0] = %s\n", parsed_input->arguments[0]);

  
  // populate arguments array 
  printf("populating elements of the array now.\n");
  fflush(stdout);
  fflush(stdin);
  //printf("before doing anything, token = %s", token);

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
      printf("exiting the array\n");
      break;
    }

    printf("do we continue here...?");
    parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
    copytoken(parsed_input->arguments[j], token);
    printf("parsed_input->arguments[%d]: %s\n", j, parsed_input->arguments[j]);

    /*
    else if (strstr(token, "\n")) {

      if(strcmp(token, "&\n") == 0) {
        parsed_input->background_process = true;
        // PUT A BREAK; STATEMENT HERE IF WE SHOULD NOT INCLUDE THE & CHARACTER IN THE ARGUMENT ARRAY
      }
      parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
      strncpy(parsed_input->arguments[j], token, strlen(token)-1);
      break;
    }

    // otherwise continue processing user input strings per normal
    parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
    strcpy(parsed_input->arguments[j], token);
    */

  }

  /* error checking. This is all good.
  However, if I try to access an array item thinking it's a string, but it's null, it will throw a seg fault on me.
  printf("User input array items: \n");
  j = 0;
  for (j=0;j<10; j++) {
    if (parsed_input->arguments[j] == NULL){
      break;
    }
    printf("%s\n", parsed_input->arguments[j]);
  }
  printf("did the newline character make it into the last array item?\n");
  */

  fflush(stdin);
  fflush(stdout);
  printf("we got here\n");

  
  char *saveptr2;
  char *token2;
  char *temp_full_input = calloc(strlen(input)+1, sizeof(char));

  // handle echo command
  strcpy(temp_full_input, parsed_input->full_text);
  printf("%s\n", temp_full_input);
  
  printf("did we get here?\n");
  
  char *ret;
  ret = strstr(temp_full_input, "echo ");
  //printf("%d", *ret);
  //printf("%d", *temp_full_input);
  
  if (ret != NULL){
      if (ret - temp_full_input == 0) {
      //printf("echo command found with space at beginning\n");
      token2 = strtok_r(temp_full_input, " ", &saveptr2);
      //strcpy(parsed_input->freetext, saveptr2);
      copytoken(parsed_input->freetext, saveptr2);
      //printf("free text is: %s", parsed_input->freetext);
    }
  }
  
  else {
  }

  return parsed_input;
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
    printf("parse input successful");
    exit(0);
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