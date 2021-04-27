/*
  Assignment 3 - Small Shell (Portfolio)
  CS 344 Operating Systems
  Spencer Neukam
  5/3/2021

  This program mimicks a shell. The commands cd, exit, and status are built in.
  All other commands are passed to the exec family of functions.

  Supported Features
  - blank lines
  - comment lines (#example comment)
  - exit
  - cd
  - status (returns the... )
  - all other standard commands
  - up to 512 arguments for one command
  - input and output redirection
  - using & at the end of a command to run a process in the background
  
  Unsupported
  - midline comments
  - probably a lot of other stuff.
*/

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
#include <sys/wait.h>
#include <stdio.h>

#define MAX_COMMAND_LEN 2048
#define MAX_ARGUMENTS 512
#define PID_INT_LENGTH 12
#define MAX_BACKGROUND_PIDS 50

// Global struct to track process ids of children (background processes)
/*
struct pids
{
  int length;
  char *pids[MAX_BACKGROUND_PIDS];
};
char *background_pids[];
int l;

for (l=0; l<MAX_BACKGROUND_PIDS; l++){

}*/


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
  Function arraylen()
  ---------------------------------
  description:
    returns the length of an array
  arg arr: pointer to an array
  returns: int
*/
/*
int arraylen(char *arr) {
  int i = 0;
  while (true)
}*/

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

  // TODO: CLEAN UP THIS CODE AND MAYBE SPLIT UP INTO SUPPORTING FUNCTIONS.

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
    
    // get the correct token
    if(j < 1){
      token = strtok_r(input, " ", &saveptr);
      copytoken(parsed_input->command, token);

      parsed_input->arguments[0] = calloc(strlen(token)+6, sizeof(char));
      sprintf(parsed_input->arguments[0], "/bin/%s", parsed_input->command);
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

    if (j > 0){
      parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
      copytoken(parsed_input->arguments[j], token);
    }
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
  char *comment;
  int is_comment;
  char *temp_full_input = calloc(MAX_INPUT+1, sizeof(char));

  strcpy(temp_full_input, parsed_input->full_text);  
  comment = strstr(temp_full_input, "#");
  
  if (comment != NULL){
    if (comment - temp_full_input == 0) {
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
    ignores input arguments beyond arg1
  arg parsed_input: command struct, the parsed input from the user
  returns: void
*/
void execute_cd(struct command *parsed_input)
{
  char *dir;
 
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
  Function execute_status()
  ---------------------------------
  description:
    executes the status command
    prints out either the exit status or the terminating signal of the last foreground process ran by your shell.
      The three built-in shell commands do not count as foreground processes
  returns: void
*/
void execute_status() {
  /*
  The status command prints out either the exit status or the terminating signal of the last foreground process ran by your shell.

  If this command is run before any foreground command is run, then it should simply return the exit status 0.

  The three built-in shell commands do not count as foreground processes for the purposes of this built-in command - i.e., status should ignore built-in commands.
  */
  printf("execute_status() called. This function is not yet implemented.");
}

/* 
  Function exec_foreground()
  ---------------------------------
  description:
    executes shell commands via exec() family of functions in the foreground.
    waits for the command to complete before returning.
  returns: void
*/
void exec_foreground(struct command *parsed_input) {
  
	int childStatus;
	pid_t spawnPid = fork(); // fork new process

	switch(spawnPid){
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0:
		// In the child process
		// Replace the current program
    execv(parsed_input->arguments[0], parsed_input->arguments);
		// exec only returns if there is an error
		perror("execve");
		exit(2);
		break;
	default:
		// In the parent process
		// Wait for child's termination
		spawnPid = waitpid(spawnPid, &childStatus, 0);
		break;
	}
}

/* 
  Function exec_background()
  ---------------------------------
  description:
    executes shell commands via exec() family of functions in the background
    returns immediately, does not wait for command to complete
  returns: void
*/
void exec_background(struct command *parsed_input) {
  //TODO: IMPLEMENT
}

/* 
  Function execute_exit()
  ---------------------------------
  description:
    exits this shell program
    kills all child processes or jobs before exiting.
  returns: void
*/
void execute_exit() {
  //TODO: IMPLEMENTE EXECUTE_EXIT() FUNCTION
  printf("execute_exit() called. This function is not yet implemented.");

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
    exit(0);
  }

  // handler for cd command
  else if (strcmp(parsed_input->command, "cd") == 0){
    execute_cd(parsed_input);
    return 1;
  }

  // handler for status command
  else if (strcmp(parsed_input->full_text, "status\n") == 0){
    execute_status();
    return 1;
  }

  // handler for foreground commands
  else if (parsed_input->background_process == false) {
    exec_foreground(parsed_input);
    return 1;
  }

  // handler for background commands
  else if (parsed_input->background_process == true) {
    exec_background(parsed_input);
    return 1;
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

    fflush(stdin);
    fflush(stdout);
    
    printf_custom(": ", 0);
    fgets(input, MAX_COMMAND_LEN+1, stdin);

    variable_expansion(input);
    user_command = parse_input(input);
    continue_ = execute(user_command);

    free(user_command);
    free(input);

  } while (continue_ == 1);
  
}

/* 
  Function killchildprocesses()
  ---------------------------------
  description:
    kills all child processes and jobs of small shell before exiting
  return: int
*/
void killchildprocesses()
{
  //TODO: IMPLEMENT
  return;
}

/* 
  execute the program
*/
int main(void) {
  //atexit(killchildprocesses);
  small_shell();
  return EXIT_SUCCESS;
}