#include "parser.h"    // cmd_t, position_t, parse_commands()

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>     //fcntl(), F_GETFL

#define READ  0
#define WRITE 1

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n) {
  for (int i = 0; i < n; i++) {
    printf("==> commands[%d]\n", i);
    printf("  pos = %s\n", position_to_string(commands[i].pos));
    printf("  in  = %d\n", commands[i].in);
    printf("  out = %d\n", commands[i].out);

    print_argv(commands[i].argv);
  }

}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd) {
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error() {
  perror("fork() failed)");
  exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i, int fd_old[], int fd_new[], position_t pos) {
  pid_t pid;

  
  switch (pid = fork()) {
    case -1:
      fork_error();
    case 0:
      // Child process after a successful fork().

      switch (pos)
      {
      case single:
        close(fd_new[0]);
        close(fd_new[1]);
        break;
      case first:
        close(fd_new[0]);
        dup2(fd_new[1], 1);
        break;
      case middle:
        close(fd_old[1]);
        close(fd_new[0]);
        dup2(fd_old[0], 0);
        dup2(fd_new[1], 1);
        break;
      case last:
        close(fd_old[1]);
        close(fd_new[0]);
        close(fd_new[1]);
        dup2(fd_old[0], 0);
        break;
      default:
        break;
      }
      
      // Execute the command in the contex of the child process.
      execvp(commands[i].argv[0], commands[i].argv);

      // If execvp() succeeds, this code should never be reached.
      fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
      exit(EXIT_FAILURE);

    default:
      // Parent process after a successful fork().

      break;
  }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n) {
  
  int fd_old[2] = {0, 0};
  for (int i = 0; i < n; i++) {
    int fd[2];
    if (pipe(fd)) exit(EXIT_FAILURE);
    position_t pos = cmd_position(i, n);
    fork_cmd(i, fd_old, fd, pos);
    if(fd_old[0] != 0){
      close(fd_old[0]);
      close(fd_old[1]);
    }
    fd_old[0] = fd[0];
    fd_old[1] = fd[1];
  }
  if (n != 0){
    close(fd_old[0]);
    close(fd_old[1]);
  }
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char* buffer, size_t size) {
  if(getline(&buffer, &size, stdin) != -1) buffer[strlen(buffer)-1] = '\0';
  else exit(EXIT_FAILURE);
}

/**
 * Make the parents wait for all the child processes.
 */
void wait_for_all_cmds(int n) {
  for(int i = 0; i < n; i++){
    wait(NULL);
  }
}

int main() {
  int n;               // Number of commands in a command pipeline.
  size_t size = 128;   // Max size of a command line string.
  char line[size];     // Buffer for a command line string.

  while(true) {
    printf(" >>> ");

    get_line(line, size);

    n = parse_commands(line, commands);

    fork_commands(n);

    wait_for_all_cmds(n);
  }

  exit(EXIT_SUCCESS);
}
