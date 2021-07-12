#include "pb_stack.h"

#define debug(i) printf(i)

FILE *read_ptr;

void restore(pid_t pid)
{
    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        exit(1);
    }
    wait(NULL);

    // read registers from a binary file
    struct user_regs_struct regs;
    read_ptr = fopen("registers.bin", "rb");
    fread(&regs, sizeof(struct user_regs_struct), 1, read_ptr);
    fclose(read_ptr);

    // set registers
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1)
    {
        printf("unable to set registers\n");
        exit(1);
    }

    // read stack from a binary file
    struct stack_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    read_ptr = fopen("data.bin", "rb");
    fread(stack_data, sizeof(unsigned char), stack_size, read_ptr);
    fclose(read_ptr);

    // write the new stack. start after the return address of main
    int i = 92;
    while (i * 4 <= stack_size)
    {
        unsigned char d[4] = {*(stack_data + i * 4), *(stack_data + i * 4 + 1), *(stack_data + i * 4 + 2), *(stack_data + i * 4 + 3)};
        putdata(pid, regs.rsp + 4 * i / 2, d, 4);
        i += 2;
    }

    // deattach from process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    // if (argc < 2)
    // {
    //     printf("Usage: %s <pid to be restored>\n", argv[0]);
    //     exit(1);
    // }

    // pid_t pid = atoi(argv[1]);
    pid_t pid = 19113;

    // pid_t pid = fork();
    // if (0 == pid)
    // {
    //     // printf("CHILED %i %i\n", pid, getpid());
    //     // ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    //     execl("/mnt/c/Users/ghaja/Desktop/university/BS2/ProcessBox/freezer_ptrace_stack_reloaded/counter", "", (char *)NULL);
    // }
    // else
    // {
    //     // printf("PARENT %i %i\n", pid, getpid());
    //     sleep(3);

    //     restore(pid);

    //     // wait for child process
    //     int status;
    //     if (wait(&status) == -1)
    //         perror("wait() error");
    //     else if (WIFEXITED(status))
    //         printf("The child exited with status of %d\n",
    //                WEXITSTATUS(status));
    //     else
    //         puts("The child did not exit successfully");
    // }

    // // read cmdline
    // char *cmdline = (char *)malloc(sizeof(char) * 10);
    // read_ptr = fopen("cmdline", "r");
    // fscanf(read_ptr, " %s", cmdline);
    // fclose(read_ptr);

    // // extract process name
    // char * process = strtok(cmdline, " ");
    // if(process[0] == '.')
    //     process ++;
    // if(process[0] == '/')
    //     process ++;
    // printf("%s", process);

    // // create a new process
    // // FIXME
    // system("cd cwd");
    // system(cmdline);

    // pid_t pid = getPidByName(process);
    restore(pid);
    
    return 0;
}