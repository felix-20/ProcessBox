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