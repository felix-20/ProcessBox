#include <wait.h>

#include "pb_files.h"
#include "pb_memory.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        printf("Options:\n\t-t Terminate the process after finishing\n\t-f Save files\n");
        exit(1);
    }

    pid_t pid = atoi(argv[1]);

    int opt;
    int terminate_process = 0, save_files = 0;
    while ((opt = getopt(argc, argv, "tf")) != -1)
    {
        switch (opt)
        {
        case 't':
            terminate_process = 1;
            break;
        case 'f':
            save_files = 1;
            break;
        }
    }

    // // save cmdline and cwd
    // char *command = (char *)malloc(100 * sizeof(char));
    // sprintf(command, "cp -r /proc/%s/cwd $cwd .", argv[1]);
    // system(command);
    // sprintf(command, "cp /proc/%s/cmdline $cmdline .", argv[1]);
    // system(command);

    // attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        perror("invalid PID\n");
        exit(1);
    }
    wait(NULL);

    save_vma(pid);
    log("VMA saved successfully\n");

    if (save_files)
    {
        // for now, save only one file
        save_file(pid);
        log("File saved successfully\n");
    }

    // stop the process if wished
    if (terminate_process)
    {
        ptrace(PTRACE_KILL, pid, NULL, NULL);
        log("Process terminated successfully\n");
    }
    // deattach from process
    else if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        perror("unable to deattach the process\n");
        return 1;
    }

    log("DONE\n");

    return 0;
}