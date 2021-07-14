#include "pb_files.h"
#include "pb_memory.h"

void parse_pb_fd(pid_t pid, struct pb_fd *cp_file)
{
    struct dirent *fd_dirent;
    struct stat stat_buf;
    DIR *proc_fd;
    char tmp_fn[1024];

    /* Go to last fd */
    snprintf(tmp_fn, 30, "/proc/%d/fd", pid);
    proc_fd = opendir(tmp_fn);
    while (1)
    {
        fd_dirent = readdir(proc_fd);
        if (fd_dirent == NULL)
            break;

        if (fd_dirent->d_type != DT_LNK)
            continue;

        cp_file->fd = atoi(fd_dirent->d_name);
    }

    /* Find out if it's open for r/w/rw */
    snprintf(tmp_fn, 1024, "/proc/%d/fd/%d", pid, cp_file->fd);
    lstat(tmp_fn, &stat_buf);

    if ((stat_buf.st_mode & S_IRUSR) && (stat_buf.st_mode & S_IWUSR))
        cp_file->mode = O_RDWR;
    else if (stat_buf.st_mode & S_IWUSR)
        cp_file->mode = O_WRONLY;
    else
        cp_file->mode = O_RDONLY;

    cp_file->offset = get_offset(pid, cp_file->fd);

    fetch_fd(pid, cp_file->fd, stat_buf, tmp_fn, cp_file);
}

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
    // pid_t pid = 4287;
    // int stop_process_afterwards = 0;

    // // save cmdline and cwd
    // char *command = (char *)malloc(100 * sizeof(char));
    // sprintf(command, "cp -r /proc/%s/cwd $cwd .", argv[1]);
    // system(command);
    // sprintf(command, "cp /proc/%s/cmdline $cmdline .", argv[1]);
    // system(command);

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

    // write registers into a binary file
    write_ptr = fopen("registers.bin", "wb");
    fwrite(&regs, sizeof(struct user_regs_struct), 1, write_ptr);
    fclose(write_ptr);

    // read stack
    struct stack_space stack_space = get_stack_space(pid);
    long long stack_size = stack_space.end - regs.rsp;
    unsigned char *stack_data = (unsigned char *)malloc(sizeof(unsigned char) * stack_size * 4);
    getdata(pid, regs.rsp, stack_data, stack_size);

    // write stack into a binary file
    write_ptr = fopen("stack.bin", "wb");
    fwrite(stack_data, sizeof(unsigned char), stack_size, write_ptr);
    fclose(write_ptr);

    // read heap
    struct heap_space heap_space = get_heap_space(pid);
    long long heap_size = heap_space.end - heap_space.start - 2000;
    printf("Heap Size: %lli\n", heap_size);
    unsigned char *heap_data = (unsigned char *)malloc(sizeof(unsigned char) * heap_size * 4);
    getdata(pid, heap_space.start + 2000, heap_data, heap_size);

    // write heap into a binary file
    write_ptr = fopen("heap.bin", "wb");
    fwrite(heap_data, sizeof(unsigned char), heap_size, write_ptr);
    fclose(write_ptr);

    // save last file
    struct pb_fd file;
    struct pb_fd *fptr = &file;
    parse_pb_fd(pid, fptr);

    // save info about file
    save_file_content_and_info(fptr, "file.backup");

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