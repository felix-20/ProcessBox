#include "pb_memory.h"
#include "pb_files.h"

#define debug(i) printf(i)

FILE *read_ptr;

void restore(pid_t pid)
{
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
}

void set_offset(pid_t pid, int fd, int offset)
{
    char command[1000];
    snprintf(command, 1000, "gdb --pid=%i --silent --batch -ex \"compile code lseek(%i,%i,0)\"", pid, fd, offset);
    system(command);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <pid to be restored>\n", argv[0]);
        exit(1);
    }

    pid_t pid = atoi(argv[1]);
    // pid_t pid = 27856;

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        exit(1);
    }
    wait(NULL);

    restore(pid);

    // deattach from process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to deattach the process\n");
        exit(1);
    }
    	
    // restore file
    struct pb_fd file;
    get_file_content_and_info(&file);
    char fn[100];
    strcpy(fn, file.filename);
    restore_file(fn, file.contents);
    restore_fd(&file, fn);
    set_offset(pid, file.fd, file.offset);

    return 0;
}