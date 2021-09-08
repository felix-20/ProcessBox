#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <wait.h>

#include "pb_files.h"
#include "pb_memory.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <pid to be restored>\n", argv[0]);
        printf("Options:\n\t-f Restore files\n");
        exit(1);
    }

    pid_t pid = atoi(argv[1]);

    int opt;
    int restore_files = 0;
    while ((opt = getopt(argc, argv, "f")) != -1)
    {
        switch (opt)
        {
        case 'f':
            restore_files = 1;
            break;
        }
    }

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        printf("invalid PID\n");
        exit(1);
    }
    wait(NULL);

    restore_vma(pid);
    log("VMA restored successfully\n");

    // detach from process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        printf("unable to detach the process\n");
        exit(1);
    }

    if (restore_files)
    {
        // for now, restore only one file
        restore_file(pid, "file.backup");
        log("File restored successfully\n");
    }
    
    log("DONE\n");

    return 0;
}