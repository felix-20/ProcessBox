#include "processbox.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <pid to be restored>\n", argv[0]);
        exit(1);
    }

    pid_t pid = atoi(argv[1]);

    restore_vma(pid);

    // restore file
    struct pb_file file;
    read_file_backup(&file, "file.backup");
    char fn[100];
    strcpy(fn, file.filename);
    restore_contents(fn, &file);
    // only use when the process didn't open the file manually
    // restore_fd(pid, &file, fn);
    restore_offset(pid, &file);

    return 0;
}