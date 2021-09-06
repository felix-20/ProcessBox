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

/*
ProcessBox File
*/
struct pb_file
{
    int fd;
    char *mode;
    off_t offset;
    int size;
    char *filename;
    char *contents;
};

/* -------- SAVE ---------- */

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

/*
Returs size of file with (fd) of process with (pid)
*/
int get_size(pid_t pid, int fd)
{
    struct stat stat_buf;
    char link_path[1024];
    snprintf(link_path, sizeof(link_path), "/proc/%d/fd/%i", pid, fd);

    lstat(link_path, &stat_buf);
    return stat_buf.st_size;
}

/*
Returs filename of file with (fd) of process with (pid)
*/
char *get_filename(pid_t pid, int fd)
{

    int bufsz = 8192;
    int retsz;
    char *buf = NULL;

    char link_path[1024];
    snprintf(link_path, sizeof(link_path), "/proc/%d/fd/%i", pid, fd);

    buf = malloc(bufsz);
    retsz = readlink(link_path, buf, bufsz);
    if (retsz <= 0)
    {
        perror("Failed to reand fd link");
        free(buf);
        exit(1);
    }
    buf[retsz] = '\0';
    return strdup(buf);
}

/*
Returs mode of file with (fd) of process with (pid)
*/
char *get_mode(pid_t pid, int fd)
{
    struct stat stat_buf;
    char link_path[1024];
    snprintf(link_path, sizeof(link_path), "/proc/%d/fd/%i", pid, fd);

    lstat(link_path, &stat_buf);
    if ((stat_buf.st_mode & S_IRUSR) && (stat_buf.st_mode & S_IWUSR))
        return "r+";
    else if (stat_buf.st_mode & S_IWUSR)
        return "w";
    else if (stat_buf.st_mode & S_IRUSR)
        return "r";
    else
    {
        perror("Can't determine access mode");
        exit(1);
    }
}

/*
Writes contents of (file).
!! filename must be set !! 
*/
void save_contents(struct pb_file *file)
{
    FILE *fptr = fopen(file->filename, "r");
    if (fptr == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    file->contents = malloc(sizeof(char) * MAXSIZE);
    fread(file->contents, sizeof(char), MAXSIZE, fptr);
    fclose(fptr);
}

/*
Parse info of file with (fd) of process with (pid) into (file)
*/

void parse_file_info(pid_t pid, int fd, struct pb_file *file){
    file->fd = fd;
    log("\t-FD: %i\n", file->fd);
    file->filename = get_filename(pid, fd);
    log("\t-Filename: %s\n", file->filename);
    file->mode = get_mode(pid, fd);
    log("\t-Mode: %s\n", file->mode);
    file->offset = get_offset(pid, fd);
    log("\t-Offset: %li\n", file->offset);
    file->size = get_size(pid, fd);
    log("\t-Size: %i\n", file->size);
    save_contents(file);
}

/*
Writes all fields of (file) into a backup file with (file_name)
*/
void save_file_info(struct pb_file *file, char *file_name)
{

    FILE *backup_file = fopen(file_name, "wb");
    fprintf(backup_file, "%i\n", file->fd);
    fprintf(backup_file, "%s\n", file->mode);
    fprintf(backup_file, "%li\n", file->offset);
    fprintf(backup_file, "%i\n", file->size);
    fprintf(backup_file, "%s\n", file->filename);
    fprintf(backup_file, "%s\n", file->contents);
    fclose(backup_file);
}

/*
Finds the fd of a file of process with pid and save it's info into a file
This takes the first file in /proc that's not stdin, stdout or stderr
*/
void save_file(pid_t pid)
{
    struct dirent *fd_dirent;
    DIR *proc_fd;
    char fd_link_path[1024];
    int fd;

    // find a file that's not stdin/stdout/stderr file
    snprintf(fd_link_path, sizeof(fd_link_path), "/proc/%d/fd", pid);
    proc_fd = opendir(fd_link_path);
    while (1)
    {
        fd_dirent = readdir(proc_fd);
        if (fd_dirent == NULL) // file not found
        {
            perror("Can't find file to save");
            exit(1);
        }

        if (fd_dirent->d_type == DT_LNK)
        {
            fd = atoi(fd_dirent->d_name);
            if (strstr(get_filename(pid, fd), "/dev/") == NULL) // file found
                break;
        }
    }

    struct pb_file file;
    parse_file_info(pid, fd, &file);
    save_file_info(&file, "file.backup");
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
    char command[1024];
    snprintf(command, 1024,
             "gdb --pid=%i --silent --batch -ex 'compile code FILE* fp = fopen(\"%s\", \"%s\");int ffd = fileno(fp);if (ffd != %i){dup2(ffd, %i);close(ffd);}'",
             pid, file_path, file->mode, file->fd, file->fd);
    system(command);
}

/*
Sets offset of process with (pid) to (file)'s offset
*/
void restore_offset(pid_t pid, struct pb_file *file)
{
    char command[1024];
    snprintf(command, 1024,
             "gdb --pid=%i --silent --batch -ex 'compile code lseek(%i,%li,0)'",
             pid, file->fd, file->offset);
    system(command);
}

/*
Reads the saved file with (filename) and writes it into (file)
*/
void read_file_backup(struct pb_file *file, char *filename)
{
    FILE *backup_file = fopen(filename, "r");
    fscanf(backup_file, "%i", &(file->fd));
    log("\t-FD: %i\n", file->fd);
    file->mode = malloc(sizeof(char) * 2);
    fscanf(backup_file, "%s", file->mode);
    log("\t-Mode: %s\n", file->mode);
    fscanf(backup_file, "%li", &(file->offset));
    log("\t-Offset: %li\n", file->offset);
    fscanf(backup_file, "%i", &(file->size));
    log("\t-Size: %i\n", file->size);

    char file_path[1024];
    char *line = NULL;
    size_t len = 0;
    fscanf(backup_file, "%s", file_path);
    len = strlen(file_path);
    file->filename = malloc(sizeof(char) * len);
    file->filename = file_path;

    getline(&line, &len, backup_file);

    // read file until the end
    getdelim(&line, &len, '\0', backup_file);
    file->contents = malloc(sizeof(char) * len);
    file->contents = line;

    fclose(backup_file);
}
