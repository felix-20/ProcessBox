#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <stdlib.h>

#define MAXSIZE 1 << 13

/*
ProcessBox File
*/
struct pb_file
{
    int fd;
    int mode;
    off_t offset;
    int size;
    char *filename;
    char *contents;
};

/*
Returns offset of file with (fd) of process with (pid)
*/
off_t get_offset(pid_t pid, int fd)
{
    FILE *fptr;
    char line[1024], fdinfo_path[64];
    snprintf(fdinfo_path, sizeof(fdinfo_path), "/proc/%d/fdinfo/%d", pid, fd);

    fptr = fopen(fdinfo_path, "r");
    if (fptr == NULL)
    {
        perror("Error opening fdinfo file");
        exit(1);
    }

    fgets(line, sizeof(line), fptr);
    return atoi(line + 5);
}

/* -------- SAVE ---------- */

/*
Returs size of file with (fd) of process with (pid)
*/
static int get_size(pid_t pid, int fd)
{
    char tmp_fn[1024];
    snprintf(tmp_fn, sizeof(tmp_fn), "/proc/%d/fd/%d", pid, fd);
    struct stat st;
    stat(tmp_fn, &st);
    return st.st_size;
}

/*
Writes contents of file with (file_path) in (file) 
*/
void save_contents(struct pb_file *file, char *file_path)
{
    int bufsz = 8192;
    int retsz;
    char *buf = NULL;

    buf = malloc(bufsz);
    retsz = readlink(file_path, buf, bufsz);
    if (retsz <= 0)
    {
        fprintf(stderr, "Error reading FD %d\n", file->fd);
        goto out;
    }
    buf[retsz] = '\0';
    file->filename = strdup(buf);

    FILE *fptr = fopen(file->filename, "r");
    if (fptr == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    file->contents = malloc(sizeof(char) * MAXSIZE);
    fread(file->contents, sizeof(char), MAXSIZE, fptr);

out:
    free(buf);
}

/*
Writes all fields of (file) into a backup file with (file_name)
*/
void write_file_backup(struct pb_file *file, char *file_name)
{

    FILE *backup_file = fopen(file_name, "wb");
    fprintf(backup_file, "%i\n", file->fd);
    fprintf(backup_file, "%i\n", file->mode);
    fprintf(backup_file, "%li\n", file->offset);
    fprintf(backup_file, "%i\n", file->size);
    fprintf(backup_file, "%s\n", file->filename);
    fprintf(backup_file, "%s\n", file->contents);
    fclose(backup_file);
}

// /* -------- RESTORE ---------- */

/*
Creates file with file name (file_path) and writes (file)'s contents in it
*/
void restore_contents(char *file_path, struct pb_file *file)
{
    FILE *fptr = fopen(file_path, "w");
    if (fptr == NULL)
    {
        perror("Error creating file");
        exit(1);
    }
    fprintf(fptr, "%s", file->contents);
    fclose(fptr);
}

/*
Makes process with (pid) opens (file) with (file_path).
Ensure that the opened file has an identical fd of (file).
!! Only use this function, if the new process didn't open the file yet !!
*/
void restore_fd(pid_t pid, struct pb_file *file, char *file_path)
{
    char *access_mode;
    switch (file->mode)
    {
    case O_RDONLY:
        access_mode = "r";
        break;
    case O_WRONLY:
        access_mode = "w";
        break;
    case O_RDWR:
        access_mode = "r+";
        break;
    }
    char command[1000];
    snprintf(command, 1000,
             "gdb --pid=%i --silent --batch -ex 'compile code FILE* fp = fopen(\"%s\", \"%s\");int ffd = fileno(fp);if (ffd != %i){dup2(ffd, %i);close(ffd);}'",
             pid, file_path, access_mode, file->fd, file->fd);
    system(command);
}

/*
Sets offset of process with (pid) to (file)'s offset
*/
void restore_offset(pid_t pid, struct pb_file *file)
{
    char command[1000];
    snprintf(command, 1000,
             "gdb --pid=%i --silent --batch -ex 'compile code lseek(%i,%li,0)'",
             pid, file->fd, file->offset);
    system(command);
}

/*
Reads the saved file.backup file and writes it into (file)
*/
void read_file_backup(struct pb_file *file)
{
    FILE *backup_file = fopen("file.backup", "r");
    fscanf(backup_file, "%i", &(file->fd));
    fscanf(backup_file, "%i", &(file->mode));
    fscanf(backup_file, "%li", &(file->offset));
    fscanf(backup_file, "%i", &(file->size));

    char file_path[100];
    char *line = NULL;
    size_t len = 0;
    fscanf(backup_file, "%s", file_path);
    len = strlen(file_path);
    file->filename = malloc(sizeof(char) * len);
    file->filename = file_path;

    getline(&line, &len, backup_file);

    // read until the end
    getdelim(&line, &len, '\0', backup_file);
    file->contents = malloc(sizeof(char) * len);
    file->contents = line;

    fclose(backup_file);
}
