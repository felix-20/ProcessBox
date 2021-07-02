#include "process_box.h"

#define debug(i) printf(i)

FILE *write_ptr;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        printf("Options:\n\t-s Stop the process\n");
        exit(1);
    }

    int stop_process_afterwards = 0;
    if (argc > 2)
        stop_process_afterwards = strcmp(argv[2], "-s") == 0;

    pid_t pid = atoi(argv[1]);
    // pid_t pid = 14440;

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        return 1;
    }
    wait(NULL);

    // get registers
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
    {
        printf("unable to fetch registers\n");
        return 1;
    }
    printf("Stack Pointer: %llx\nBase Pointer: %llx\n", regs.rsp, regs.rbp);

    // write stack register into a binary file
    write_ptr = fopen("registers.bin", "wb");
    fprintf(write_ptr, " %llu", regs.rsp);
    fclose(write_ptr);

    // read stack
    struct stack_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    getdata(pid, regs.rsp, stack_data, stack_size);

    // write stack into a binary file
    write_ptr = fopen("data.bin", "wb");
    fwrite(stack_data, sizeof(unsigned char), stack_size, write_ptr);
    fclose(write_ptr);

    // stop the process if wished
    if (stop_process_afterwards)
    {
        ptrace(PTRACE_KILL, pid, NULL, NULL);
    }
    // deattach from process
    else if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        return 1;
    }

    return 0;
}