#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

// to fork a process

int do_fork_wait(int argc, char *argv[])
{
  pid_t pid;
  if ((pid = fork()) < 0)
  {
    exit(0);
  }
  else if (pid == 0)
  {
    /* Kindprozess */
    char *command_and_arguments[argc + 1];

    for (int i = 0; i < argc; i++)
    {
      command_and_arguments[i] = argv[i];
    }
    command_and_arguments[argc] = NULL;

    execvp(argv[0], command_and_arguments);
    exit(0);
  }
  else
  {
    /* Elternprozess */
    int status;
    waitpid(pid, &status, 0);
  }
  return 1;
}

int main(int argc, char **argv)
{
  char cwd[50];
  if (getcwd(cwd, sizeof(cwd)) == NULL)
  {
    printf("error by parsing cwd");
    exit(1);
  }

  char proc_path[100];
  char command[20];

  snprintf(proc_path, sizeof(proc_path), "%s/%s/cmdline", cwd, argv[1]);
  // printf("%s", proc_path);

  FILE *fp = fopen(proc_path, "r");
  fgets(command, 20, (FILE*)fp);
  // printf("%s\n", command);

  proc_path[0] = '\0';

  snprintf(proc_path, sizeof(proc_path), "%s/%s/cwd", cwd, argv[1]);
  chdir(proc_path);

  char *arguments[2];
  arguments[0] = command;
  arguments[1] = 0;

  do_fork_wait(4, arguments);
  return 0;
}