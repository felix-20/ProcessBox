#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "pb_memory.h"

void getdata(pid_t pid, long addr, unsigned char *data, int len)
{
    int p = 0;
    unsigned char *laddr;
    union u
    {
        long val;
        unsigned char chars[long_size];
    } ptrace_res;
    int i, j;
    i = 0;
    j = len / long_size;
    laddr = data;
    while (i < j)
    {
        ptrace_res.val = ptrace(PTRACE_PEEKDATA, pid,
                                addr + i * 4, NULL);
        if (ptrace_res.val == -1)
        {
            perror("unable to read data %i\n", p);
            exit(1);
        }
        p += 1;
        memcpy(laddr, ptrace_res.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        ptrace_res.val = ptrace(PTRACE_PEEKDATA, pid,
                                addr + i * 4, NULL);
        memcpy(laddr, ptrace_res.chars, j);
    }
    data[len] = '\0';
}

void putdata(pid_t pid, long addr, unsigned char *data, int len)
{
    unsigned char *laddr;
    union u
    {
        long val;
        unsigned char chars[long_size];
    } ptrace_res;
    int i, j;
    i = 0;
    j = len / long_size;
    laddr = data;
    while (i < j)
    {
        memcpy(ptrace_res.chars, laddr, long_size);
        if (ptrace(PTRACE_POKEDATA, pid,
                   addr + i * 4, ptrace_res.val))
        {
            perror("unable to write data\n");
            exit(1);
        }
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        memcpy(ptrace_res.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, pid,
               addr + i * 4, ptrace_res.val);
    }
}

struct vma_space get_stack_space(pid_t pid)
{
    FILE *fptr;
    char line[1024], proc_path[64];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pid);
    fptr = fopen(proc_path, "r");
    if (fptr == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct vma_space sp;
    do
    {
        if (strstr(line, "[stack]") != NULL) // line of stack found
        {
            return sp;
        }
        fscanf(fptr, "%lx-%lx", &sp.start, &sp.end);
    } while (fgets(line, sizeof(line), fptr) != NULL);
    perror("Error finding stack");
    exit(1);
}

struct vma_space get_heap_space(pid_t pid)
{
    FILE *fptr;
    char line[2048], proc_path[50];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pid);
    fptr = fopen(proc_path, "r");
    if (fptr == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct vma_space hp;
    do
    {
        if (strstr(line, "[heap]") != NULL) // line with heap found
        {
            return hp;
        }
        fscanf(fptr, "%lx-%lx", &hp.start, &hp.end);
    } while (fgets(line, 2048, fptr) != NULL);
    perror("Error finding heap");
    exit(1);
}

void save_vma(pid_t pid)
{
    FILE *fptr;

    // get registers
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
    {
        perror("unable to fetch registers\n");
        exit(1);
    }

    // write registers into a binary file
    fptr = fopen("registers.bin", "wb");
    fwrite(&regs, sizeof(struct user_regs_struct), 1, fptr);
    fclose(fptr);
    log("Registers saved successfully\n");

    // read stack
    struct vma_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    getdata(pid, regs.rsp, stack_data, stack_size);

    // write stack into a binary file
    fptr = fopen("stack.bin", "wb");
    fwrite(stack_data, sizeof(unsigned char), stack_size, fptr);
    fclose(fptr);
    log("Stack saved successfully\n");

    // read heap
    struct vma_space heap_space = get_heap_space(pid);
    long long heap_size = heap_space.end - heap_space.start - 2000;
    unsigned char *heap_data = (unsigned char *)malloc(sizeof(unsigned char) * heap_size * 4);
    getdata(pid, heap_space.start + 2000, heap_data, heap_size);

    // write heap into a binary file
    fptr = fopen("heap.bin", "wb");
    fwrite(heap_data, sizeof(unsigned char), heap_size, fptr);
    fclose(fptr);
    log("Heap saved successfully\n");
}

void restore_vma(pid_t pid)
{
    FILE *fptr;

    // read registers from a binary file
    struct user_regs_struct regs;
    fptr = fopen("registers.bin", "rb");
    fread(&regs, sizeof(struct user_regs_struct), 1, fptr);
    fclose(fptr);

    // set registers
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1)
    {
        perror("unable to set registers\n");
        exit(1);
    }
    log("Registers restored successfully\n");

    // read stack from a binary file
    struct vma_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    fptr = fopen("stack.bin", "rb");
    fread(stack_data, sizeof(unsigned char), stack_size, fptr);
    fclose(fptr);

    // stack_offset: area before return addres of main
    // can't be overwritten due to stack canaries
    int stack_offset = 92;
    // write the new stack
    putdata(pid, regs.rsp + stack_offset * 2, stack_data + (4 * stack_offset), stack_size - (stack_offset));
    log("Stack restored successfully\n");

    // read heap from a binary file
    struct vma_space heap_space = get_heap_space(pid);
    long long heap_size = heap_space.end - heap_space.start;
    unsigned char *heap_data = (unsigned char *)malloc(sizeof(unsigned char) * heap_size * 4);
    fptr = fopen("heap.bin", "rb");
    fread(heap_data, sizeof(unsigned char), heap_size, fptr);
    fclose(fptr);

    // heap_offset: secured area of heap, can't be overwritten
    int heap_offset = 2000;
    // write the new heap
    putdata(pid, heap_space.start + 2 * heap_offset, heap_data + (4 * heap_offset), heap_size - heap_offset);
    log("Heap restored successfully\n");
}