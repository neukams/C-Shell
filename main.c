/*
  Assignment 3 - Small Shell (Portfolio)
  CS 344 Operating Systems
  Spencer Neukam
  5/3/2021

  This program mimicks a shell. The commands cd, exit, and status are built in.
  All other commands are passed to the exec family of functions.

  Supported input format:
  command [arg1 arg2 ...] [< input_file] [> output_file] [&]

  Supported Features
  - blank lines
  - comment lines (#this is an example comment line, it must start with #)
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


struct command
{
    char *full_text;
    char *command;
    char *arguments[MAX_ARGUMENTS];
    char *redirect_in;
    char *redirect_out;
    bool background_process;
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
  return;
}

/* 
  Function get_command_struct()
  ---------------------------------
  description:
    returns a command struct
    - memory allocated for char pointers
    - arguments char array values initialized to null
  returns: struct command
*/
struct command *get_command_struct()
{
  // allocate memory
  struct command *input_struct = malloc(sizeof(struct command));
  input_struct->full_text = calloc(MAX_INPUT + 1, sizeof(char));
  input_struct->command = calloc(MAX_INPUT + 1, sizeof(char));
  input_struct->redirect_in = calloc(MAX_INPUT + 1, sizeof(char));
  input_struct->redirect_out = calloc(MAX_INPUT + 1, sizeof(char));

  // initialize array arguments to null
  int i;
  for (i=0;i<MAX_ARGUMENTS;i++){
      input_struct->arguments[i] = NULL;
  }
  
  return input_struct;
}

/* 
  Function get_command_struct()
  ---------------------------------
  description:
    copies the raw user input into the ->full_text struct variable, by reference
  returns: void
*/
void set_full_text(struct command *parsed_input, char *input)
{
  strcpy(parsed_input->full_text, input);
  return;
}

/* 
  Function set_command_arguments()
  ---------------------------------
  description:
    process user arguments into the arguments array in the command struct
      user enters: ls /folder1/folder2
      arguments array: {"ls", "/folder1/folder2", NULL, NULL, ...}
      user enters: ls > cat &
      arguments array: {"ls", NULL, NULL, ...}
    Note: the arguments array is initialized with MAX_ARGUMENTS # of values. This is OK to pass into the exec family of functions, because they stop at the first NULL value. This can be modified later if need be.
    All processing is done by reference
    Does NOT process I/O redirection, or background processing into the arguments array.
  arg: parsed_input: command struct with user input to process
  arg: user_input: char pointer to user's command line input
  returns: void
*/
void set_command_arguments(struct command *parsed_input, char *user_input)
{
  char *token;
  char *saveptr;
  char *input;
  int j;

  input = calloc(MAX_INPUT+1, sizeof(char));
  strcpy(input, user_input);

  // process user arguments into the arguments array
  for (j=0;j<MAX_ARGUMENTS; j++){
    
    // get token
    if(j < 1){
      token = strtok_r(input, " ", &saveptr);
      copytoken(parsed_input->command, token);      
    }
    else if (j > 0) {
      token = strtok_r(NULL, " ", &saveptr);
    }

    // exit loop scenarios
    // - end of input (by null or by newline)
    // - we have reached file redirection statements
    // - we have reached background processing statements
    if (token == NULL) {
      break;
    }
    else if (strcmp(token, "\n") == 0) {
      break;
    }
    else if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0) {
      break;
    }
    else if (strcmp(token, "&") == 0) {
      break;
    }

    // place into arguments array
    parsed_input->arguments[j] = calloc(strlen(token)+1, sizeof(char));
    copytoken(parsed_input->arguments[j], token);
  }

  free(input);
  return;
}

/* 
  Function set_io_redirection()
  ---------------------------------
  description:
    given a command struct with the user's full command line input in the ->full_text variable, processes any i/o redirection into the argument's appropriate struct fields
  arg: parsed_input: command struct with user input (->full_text) to process
  returns: void
*/
void set_io_redirection(struct command *parsed_input)
{
  char *token;
  char *saveptr;
  char *prevtoken = calloc(MAX_INPUT+1, sizeof(char));
  char *input = calloc(strlen(parsed_input->full_text)+1, sizeof(char));
  strcpy(input, parsed_input->full_text);

  // get second token from input string (assuming first token is the command)
  token = strtok_r(input, " ", &saveptr);
  copytoken(prevtoken, token);
  token = strtok_r(NULL, " ", &saveptr);

  // get and test tokens for i/o redirection statements
  while (token) {

    // input redirection filename
    if (strcmp(prevtoken, "<") == 0) {
      copytoken(parsed_input->redirect_in, token);
      printf("redirect in: %s", parsed_input->redirect_in);
    }
    // output redirection filename
    else if (strcmp(prevtoken, ">") == 0) {
      copytoken(parsed_input->redirect_out, token);
      printf("redirect out: %s", parsed_input->redirect_out);
    }

    // iterate to next token
    copytoken(prevtoken, token);
    token = strtok_r(NULL, " ", &saveptr);
  }

  free(input);
  free(prevtoken);
  return;
}

/* 
  Function set_background_process()
  ---------------------------------
  description:
    given a command struct with parsed input for the ->full_text and ->arguments variables, update the ->background_process boolean
  arg: parsed_input: command struct with user input to process
  returns: void
*/
void set_background_process(struct command *parsed_input)
{

  if (strstr(parsed_input->full_text, "&\n") || strstr(parsed_input->full_text, "& \n")) {
    parsed_input->background_process = true;
  }
  else {
    parsed_input->background_process = false;
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
  struct command *parsed_input;

  parsed_input = get_command_struct();
  set_full_text(parsed_input, input);
  set_command_arguments(parsed_input, input);
  set_io_redirection(parsed_input);
  set_background_process(parsed_input);

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
  char *dir = calloc(strlen(parsed_input->arguments[1])+1, sizeof(char));
 
  // get directory to change to
  if (strstr(parsed_input->arguments[0], "cd") != NULL && parsed_input->arguments[1] ==  NULL) {
    dir = getenv("HOME");
  }
  else if (parsed_input->arguments[1] != NULL) {
    copytoken(dir, parsed_input->arguments[1]);
  }
  else{
    printf("invalid directory\n");
  }
 
  // change directory
  chdir(dir);
  free(dir);
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
  Function exec_foreground()
  ---------------------------------
  description:
    executes shell commands via exec() family of functions in the foreground.
    waits for the command to complete before returning.
  returns: void
*/
void exec_foreground(struct command *parsed_input) {
  
	int childStatus;
  int in_fd;
  int out_fd;
	pid_t spawnPid = fork(); // fork new process

	switch(spawnPid){
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0:
		// In the child process
    // If applicable, handle I/O redirection
    if (parsed_input->redirect_in != NULL) {
      in_fd = open(parsed_input->redirect_in, O_RDONLY);
      dup2(in_fd, 0);
    }
    if (parsed_input->redirect_out != NULL) {
      out_fd = open(parsed_input->redirect_out, O_RDWR | O_CREAT | O_TRUNC, 0640);
      dup2(out_fd, 1);
    }
    //close(in_fd);
    //close(out_fd);
		// Replace the current program
    execvp(parsed_input->arguments[0], parsed_input->arguments);
		perror("execvp");
		//exit(2);
		break;
	default:
		// In the parent process
		// Wait for child's termination
    //printf("Parent process completed");
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
  Function execute()
  ---------------------------------
  description:
    executes the command entered by the user
  arg parsed_input: command struct, the parsed input from the user
  returns: ?
*/
int execute(struct command *parsed_input)
{
  // handler for comment lines
  if (is_comment(parsed_input) == 1) {
    return 1;
  }

  // handler for empty lines
  else if (strcmp(parsed_input->full_text, "\n") == 0) {
    return 1;
  }

  // hanlder for exit command
  else if (strcmp(parsed_input->arguments[0], "exit") == 0 && parsed_input->arguments[1] == NULL) {
    execute_exit();
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
  
  printf("error, unhandled input\n");
  return 0;
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