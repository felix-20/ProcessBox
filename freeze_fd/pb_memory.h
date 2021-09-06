#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define log printf

#define MAXSIZE 1 << 13
#define BUF_SIZE 1024
struct stack_space
{
    long start, end;
};

struct heap_space
{
    long start, end;
};

const int long_size = sizeof(long);

void getdata(pid_t pid, long addr, unsigned char *str, int len)
{

    int p = 0;
    unsigned char *laddr;
    int i, j;
    union u
    {
        long val;
        unsigned char chars[long_size];
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
void putdata(pid_t pid, long addr, unsigned char *str, int len)
{
    unsigned char *laddr;
    int i, j;
    union u
    {
        long val;
        unsigned char chars[long_size];
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

struct heap_space get_heap_space(pid_t pid)
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
    struct heap_space hp;
    do
    {
        if (strstr(line, "[heap]") != NULL)
        {
            return hp;
        }
        fscanf(fp, "%lx-%lx", &hp.start, &hp.end);
    } while (fgets(line, 2048, fp) != NULL);
    perror("Error finding heap");
    exit(1);
}

void save_vma(pid_t pid)
{
    FILE *write_ptr;

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        perror("invalid PID\n");
        exit(1);
    }
    log("Process attached successfully\n");
    wait(NULL);

    // get registers
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
    {
        printf("unable to fetch registers\n");
        exit(1);
    }

    // write registers into a binary file
    write_ptr = fopen("registers.bin", "wb");
    fwrite(&regs, sizeof(struct user_regs_struct), 1, write_ptr);
    fclose(write_ptr);
    log("Registers saved successfully\n");

    // read stack
    struct stack_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    getdata(pid, regs.rsp, stack_data, stack_size);

    // write stack into a binary file
    write_ptr = fopen("stack.bin", "wb");
    fwrite(stack_data, sizeof(unsigned char), stack_size, write_ptr);
    fclose(write_ptr);
    log("Stack saved successfully\n");

    // read heap
    struct heap_space heap_space = get_heap_space(pid);
    long long heap_size = heap_space.end - heap_space.start - 2000;
    unsigned char *heap_data = (unsigned char *)malloc(sizeof(unsigned char) * heap_size * 4);
    getdata(pid, heap_space.start + 2000, heap_data, heap_size);

    // write heap into a binary file
    write_ptr = fopen("heap.bin", "wb");
    fwrite(heap_data, sizeof(unsigned char), heap_size, write_ptr);
    fclose(write_ptr);
    log("Heap saved successfully\n");
}

void restore_vma(pid_t pid)
{
    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        exit(1);
    }
    wait(NULL);

    FILE *read_ptr;
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
    read_ptr = fopen("stack.bin", "rb");
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

    // read heap from a binary file
    struct heap_space heap_space = get_heap_space(pid);
    long long heap_size = heap_space.end - heap_space.start - 2000;
    unsigned char *heap_data = (unsigned char *)malloc(sizeof(unsigned char) * heap_size * 4);
    read_ptr = fopen("heap.bin", "rb");
    fread(heap_data, sizeof(unsigned char), heap_size, read_ptr);
    fclose(read_ptr);

    // write the new heap. start after the return address of main
    i = 0;
    while (i * 4 <= heap_size)
    {
        unsigned char d[4] = {*(heap_data + i * 4), *(heap_data + i * 4 + 1), *(heap_data + i * 4 + 2), *(heap_data + i * 4 + 3)};
        putdata(pid, heap_space.start + 2000 + 4 * i / 2, d, 4);
        i += 2;
    }

    // deattach from process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        exit(1);
    }
}