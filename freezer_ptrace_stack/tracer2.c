#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct stack_space
{
    long start, end;
};

pid_t traced_process;
struct user_regs_struct regs;
long ins;
struct stack_space stack_space;
long stack_size;
unsigned char *stack_data;

const int long_size = sizeof(long);
void getdata(pid_t pro, long addr,
             char *str, int len)
{
    char *laddr;
    int i, j;
    union u
    {
        long val;
        char chars[long_size];
    } data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j)
    {
        data.val = ptrace(PTRACE_PEEKDATA, pro,
                          addr + i * 4, NULL);
        if (data.val == -1)
        {
            printf("unable to read data");
            exit(1);
        }
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        data.val = ptrace(PTRACE_PEEKDATA, pro,
                          addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}
void putdata(pid_t pro, long addr,
             char *str, int len)
{
    char *laddr;
    int i, j;
    union u
    {
        long val;
        char chars[long_size];
    } data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j)
    {
        memcpy(data.chars, laddr, long_size);
        if (ptrace(PTRACE_POKEDATA, pro,
                   addr + i * 4, data.val))
        {
            printf("unable to write data");
            exit(1);
        }
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, pro,
               addr + i * 4, data.val);
    }
}

struct stack_space get_stack_space(pid_t pro)
{
    FILE *fp;
    char line[2048], proc_path[50];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pro);
    fp = fopen(proc_path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct stack_space sp;
    do
    {
        fscanf(fp, "%lx-%lx", &sp.start, &sp.end);
        if (strstr(line, "stack") != NULL)
        {
            return sp;
        }
    } while (fgets(line, 2048, fp) != NULL);
    perror("Error finding stack");
    exit(1);
}

void fork_counter()
{
    pid_t pid;
    if ((pid = fork()) < 0)
    {
        printf("unable to create a new process\n");
        exit(0);
    }
    else if (pid == 0)
    {
        execvp("/mnt/c/Users/ghaja/Desktop/university/BS2/step2/counter", 0);
    }
    else
    {
        // wait(NULL);
        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
        {
            printf("unable to trace child process");
            exit(1);
        }
        printf("child process %i\n", pid);

        // putdata(pid, stack_space.start, stack_data, 4 * stack_size);
        // ptrace(PTRACE_SETREGS, pid, NULL, &regs);

        char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        putdata(pid, regs.rsp + 1000, data, 10);
        // getdata(pid, 0x7ffe919f9000, data, 10);

        for (int i = 0; i < 10; i++)
            printf("%i ## ", data[i]);

        // ptrace(PTRACE_CONT, pid, NULL, NULL);
        // ptrace(PTRACE_DEATTACH, pid, NULL, NULL);
        // int status;
        // waitpid(pid, &status, 0);

        printf("#### %li ####\n", ptrace(PTRACE_KILL, pid, NULL, NULL));
        int status;
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <pid to be traced>\n",
               argv[0]);
        exit(1);
    }

    traced_process = atoi(argv[1]);

    // attach to the process
    if (ptrace(PTRACE_ATTACH, traced_process, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        return 1;
    }
    wait(NULL);

    // get registers
    if (ptrace(PTRACE_GETREGS, traced_process, NULL, &regs) == -1)
    {
        printf("unable to fetch registers\n");
        return 1;
    }
    printf("Stack Pointer: %llx\nBase Pointer: %llx\n", regs.rsp, regs.rbp);

    // load stack from the process
    stack_space = get_stack_space(traced_process);
    stack_size = (stack_space.end - stack_space.start);
    stack_data = (char *)malloc(sizeof(char) * stack_size * 4);
    printf("Stack Range: %lx - %lx\n", stack_space.start, stack_space.end);

    // struct iovec local[1];
    // struct iovec remote[1];
    // ssize_t nread;

    // local[0].iov_base = stack_data;
    // local[0].iov_len = 4 * stack_size;
    // remote[0].iov_base = (void *)stack_space.start;
    // remote[0].iov_len =  4 * stack_size;

    // nread = process_vm_readv(traced_process, local, 1, remote, 1, 0);
    // if (nread != stack_size)
    // {
    //     printf("had a problem with reading the stack got %li instead of %li\n", nread, stack_size);
    //     exit(0);
    // }

    // getdata(traced_process, stack_space.start, stack_data, 4 * stack_size);

    // for (int i = 0; i< 100; i++)
    // {
    //     if(stack_data[i*4] != 0)
    //     printf("0x%02x%02x%02x%02x\n", *(stack_data + 4 * i + 3), *(stack_data + 4 * i + 2), *(stack_data + 4 * i + 1), *(stack_data + 4 * i + 0));
    // }

    // deattach from the process and kill it
    if (ptrace(PTRACE_DETACH, traced_process, NULL, NULL) == -1)
    {
        printf("unable to deattach from the process\n");
        return 1;
    }
    // ptrace(PTRACE_KILL, traced_process, NULL, NULL);

    // run a new process
    // fork_counter();

    // ptrace(PTRACE_DETACH, pid, NULL, NULL);

    // wait until child process is done
    // int status;
    // waitpid(pid, &status, 0);
    // putdata(pid, stack_space.start, stack_data, stack_size);

    return 0;
}