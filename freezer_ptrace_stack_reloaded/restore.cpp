#include "process_box.h"

#define debug(i) printf(i)

FILE *read_ptr;

int main(int argc, char *argv[])
{
    // if (argc < 2)
    // {
    //     printf("Usage: %s <pid to be restored>\n", argv[0]);
    //     exit(1);
    // }

    pid_t pid = atoi(argv[1]);
    // pid_t pid = 23238;

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        return 1;
    }
    wait(NULL);


    // read stack register from a binary file
    unsigned long long rsp;
    read_ptr = fopen("registers.bin", "rb");
    fscanf(read_ptr, " %llu", &rsp);
    fclose(read_ptr);

    // read stack from a binary file
    struct stack_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    read_ptr = fopen("data.bin", "rb");
    fread(stack_data, sizeof(unsigned char), stack_size, read_ptr);
    fclose(read_ptr);

    // write the new stack. start after the return address of main
    int i = 92;
    while (i * 4 <= stack_size)
    {
        unsigned char d[4] = {*(stack_data + i * 4), *(stack_data + i * 4 + 1), *(stack_data + i * 4 + 2), *(stack_data + i * 4 + 3)};
        putdata(pid, rsp + 4 * i/2, d, 4);
        i += 2;
    }

    // deattach from process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        return 1;
    }

    return 0;
}