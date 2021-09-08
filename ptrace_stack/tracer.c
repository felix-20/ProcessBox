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

pid_t old_process, new_process;
struct user_regs_struct regs;
long ins;
struct stack_space stack_space;
long stack_size;
unsigned char *stack_data;

const int long_size = sizeof(long);

void getdata(pid_t pid, long addr, char *str, int len)
{

    int p = 0;
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
        data.val = ptrace(PTRACE_PEEKDATA, pid,
                          addr + i * 4, NULL);
        if (data.val == -1)
        {
            printf("unable to read data %i\n", p);
            exit(1);
        }
        p += 1;
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        data.val = ptrace(PTRACE_PEEKDATA, pid,
                          addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}
void putdata(pid_t pid, long addr, char *str, int len)
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
        if (ptrace(PTRACE_POKEDATA, pid,
                   addr + i * 4, data.val))
        {
            printf("unable to write data\n");
            exit(1);
        }
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, pid,
               addr + i * 4, data.val);
    }
}

struct stack_space get_stack_space(pid_t pid)
{
    FILE *fp;
    char line[2048], proc_path[50];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pid);
    fp = fopen(proc_path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct stack_space sp;
    do
    {
        if (strstr(line, "[stack]") != NULL)
        {
            return sp;
        }
        fscanf(fp, "%lx-%lx", &sp.start, &sp.end);
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
        // system("/mnt/c/Users/ghaja/Desktop/university/BS2/step2/counter");
        char args[] = {0};
        execl("/mnt/c/Users/ghaja/Desktop/university/BS2/step2/counter", "/mnt/c/Users/ghaja/Desktop/university/BS2/step2/counter", (char *)NULL);
    }
    else
    {
        // wait(NULL);
        printf("Child Process ID: %i\n", pid);
        printf("%i\n", getpid());
        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
        {
            printf("invalid PID\n");
            exit(1);
        }
        // wait(NULL);
        if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1)
        {
            printf("unable to set registers\n");
            exit(1);
        }

        int i = 98;
        char d[4] = {*(stack_data + i * 4), *(stack_data + i * 4 + 1), *(stack_data + i * 4 + 2), *(stack_data + i * 4 + 3)};
        // printf("%02x %02x %02x %02x", *(stack_data + i*4), *(stack_data + i*4 + 1), *(stack_data + i*4 + 2), *(stack_data + i*4 + 3));
        // putdata(pid, regs.rsp + 4 * 49, d, 4);
        // wait(NULL);
        if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
        {
            printf("unable to deattach the process\n");
            exit(1);
        }

        // wait(NULL);
        int status;
        if (wait(&status) == -1)
            perror("wait() error");
        else if (WIFEXITED(status))
            printf("The child exited with status of %d\n",
                   WEXITSTATUS(status));
        else
            puts("The child did not exit successfully");
    }
}

int main(int argc, char *argv[])
{
    // if (argc != 2)
    // {
    //     printf("Usage: %s <pid to be traced>\n",
    //            argv[0]);
    //     exit(1);
    // }

    old_process = atoi(argv[1]);
    new_process = atoi(argv[2]);
    // old_process = 5637;

    // old_process = 8587;

    // attach to the process
    if (ptrace(PTRACE_ATTACH, old_process, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        return 1;
    }
    wait(NULL);

    // get registers
    if (ptrace(PTRACE_GETREGS, old_process, NULL, &regs) == -1)
    {
        printf("unable to fetch registers\n");
        return 1;
    }
    printf("Stack Pointer: %llx\nBase Pointer: %llx\n", regs.rsp, regs.rbp);

    stack_space = get_stack_space(old_process);
    stack_size = (stack_space.end - regs.rsp);
    stack_data = (char *)malloc(sizeof(char) * stack_size * 4);
    printf("Filled Stack Size: %li\n", stack_size);
    getdata(old_process, regs.rsp, stack_data, stack_size);

    // for (int i = 0; i< 200; i+=2)
    // {
    //     // if(stack_data[i*4] != 0)
    //     printf("%i 0x%02x%02x%02x%02x\n", i*4, *(stack_data + 4 * i + 3), *(stack_data + 4 * i + 2), *(stack_data + 4 * i + 1), *(stack_data + 4 * i + 0));
    // }

    // char d[4] = {0x00, 0xFF, 0xFF, 0x00};
    // putdata(old_process, regs.rsp + 49 * 4, d, 4);
    // // putdata(old_process, regs.rsp + 3 * 4, d, 4);
    // getdata(old_process, regs.rsp, stack_data, stack_size);
    // printf("##############");

    // for (int i = 0; i< 200; i+=2)
    // {
    //     // if(stack_data[i*4] != 0)
    //     printf("%i 0x%02x%02x%02x%02x\n", i*4, *(stack_data + 4 * i + 3), *(stack_data + 4 * i + 2), *(stack_data + 4 * i + 1), *(stack_data + 4 * i + 0));
    // }
    // int i = 98;
    // printf("%i 0x%02x%02x%02x%02x\n", i*4, *(stack_data + 4 * i + 3), *(stack_data + 4 * i + 2), *(stack_data + 4 * i + 1), *(stack_data + 4 * i + 0));

    // deattach from the process and kill it
    if (ptrace(PTRACE_DETACH, old_process, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        return 1;
    }
    // ptrace(PTRACE_KILL, old_process, NULL, NULL);

    if (ptrace(PTRACE_ATTACH, new_process, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        return 1;
    }
    wait(NULL);
    if (ptrace(PTRACE_SETREGS, new_process, NULL, &regs) == -1)
    {
        printf("unable to set registers\n");
        return 1;
    }

    int i = 92;
    while (i * 4 <= stack_size)
    {
        char d[4] = {*(stack_data + i * 4), *(stack_data + i * 4 + 1), *(stack_data + i * 4 + 2), *(stack_data + i * 4 + 3)};
        putdata(new_process, regs.rsp + 4 * i/2, d, 4);
        printf("%02x %02x %02x %02x", *(stack_data + i*4), *(stack_data + i*4 + 1), *(stack_data + i*4 + 2), *(stack_data + i*4 + 3));
        printf(" Done at %i\n", i);
        i += 2;
    }
    if (ptrace(PTRACE_DETACH, new_process, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        return 1;
    }

    // run a new process
    // fork_counter();

    // ptrace(PTRACE_DETACH, pid, NULL, NULL);

    // wait until child process is done
    // int status;
    // waitpid(pid, &status, 0);
    // putdata(pid, stack_space.start, stack_data, stack_size);

    return 0;
}