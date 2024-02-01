#include <stdio.h>    // puts(), printf(), perror(), getchar()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>   // getpid(), getppid(),fork()
#include <sys/wait.h> // wait()

#define READ  0
#define WRITE 1

void child_a(int fd[]) {

  close(fd[READ]);
  dup2(fd[WRITE], WRITE);

  execlp("ls", "ls", "-F", "-l", NULL);

  exit(EXIT_FAILURE);

}

void child_b(int fd[]) {

  close(fd[WRITE]);
  dup2(fd[READ], READ);

  execlp("nl", "nl", NULL);

  exit(EXIT_FAILURE);
}

int main(void) {
  int fd[2];

  if (pipe(fd) == -1) exit(EXIT_FAILURE);

  int a_id = fork();

  if(a_id == 0){
    child_a(fd);
  }
  else if(a_id == -1){
    exit(EXIT_FAILURE);
  }

  int b_id = fork();

  if(b_id == 0){
    child_b(fd);
  }
  else if (b_id == -1) {
    exit(EXIT_FAILURE);
  }

  close(fd[READ]);
  close(fd[WRITE]);

  int status;
  wait(&status);
  wait(&status);
  puts("Done");
}
