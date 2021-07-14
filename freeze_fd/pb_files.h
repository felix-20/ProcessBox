#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <stdlib.h>

/* The possibilities for the third argument to `setvbuf'.  */
#define _IOFBF 0 /* Fully buffered.  */
#define _IOLBF 1 /* Line buffered.  */
#define _IONBF 2 /* No buffering.  */

/* Default buffer size.  */
#define BUFSIZ 8192

/* Some possibly not declared defines */
#ifndef O_DIRECT
#define O_DIRECT 040000 /* direct disk access hint */
#endif                  /* O_DIRECT */
#ifndef O_NOATIME
#define O_NOATIME 01000000
#endif /* O_NOATIME */

struct pb_fd
{
    int fd;
    int mode;
    off_t offset;
    int size;
    char *filename;
    char *contents;
};

off_t get_offset(pid_t pid, int fd)
{
    FILE *fp;
    char line[2048], fdinfo_path[50];
    snprintf(fdinfo_path, sizeof(fdinfo_path), "/proc/%d/fdinfo/%d", pid, fd);
    fp = fopen(fdinfo_path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    fgets(line, 2048, fp);
    return atoi(line + 5);
}

/* -------- SAVE ---------- */

static int get_file_size(pid_t pid, int fd)
{
    char tmp_fn[1024];
    snprintf(tmp_fn, 1024, "/proc/%d/fd/%d", pid, fd);
    struct stat st;
    stat(tmp_fn, &st);
    return st.st_size;
}

void fetch_fd(pid_t pid, int fd, struct stat file_stat, char *fd_path,
              struct pb_fd *file)
{
    int bufsz = 512;
    int retsz;
    char *buf = NULL;

    file->filename = NULL;
    file->size = file_stat.st_size;

    do
    {
        buf = malloc(bufsz);
        retsz = readlink(fd_path, buf, bufsz);
        if (retsz <= 0)
        {
            fprintf(stderr, "Error reading FD %d\n", fd);
            goto out;
        }
        else if (retsz < bufsz)
        {
            /* Read was successful */
            buf[retsz] = '\0';
            file->filename = strdup(buf);
            break;
        }
        /* Otherwise, double the buffer size and try again */
        free(buf);
        bufsz <<= 1;
    } while (bufsz <= 8192); /* Keep it sane */

    FILE *fptr = fopen(file->filename, "r");
    file->contents = malloc(sizeof(char) * 2000);
    fread(file->contents, sizeof(char), 2000, fptr);

out:
    free(buf);
}

// /* -------- RESTORE ---------- */

void restore_file(char *fn, char *contents)
{
    // printf("%i", remove(file->filename));
    FILE *fptr = fopen(fn, "w");
    fprintf(fptr, contents);
    fclose(fptr);
}

void restore_fd(struct pb_fd *file, char *fn)
{
    int ffd;
    ffd = open(fn, file->mode);

    if (ffd != file->fd)
    {
        dup2(ffd, file->fd);
        close(ffd);
    }
    lseek(ffd, 0, SEEK_END);
}

void save_file_content_and_info(struct pb_fd *file, char *file_location)
{

    FILE *backup_file = fopen(file_location, "wb");
    fprintf(backup_file, "%i\n", file->fd);
    fprintf(backup_file, "%i\n", file->mode);
    fprintf(backup_file, "%li\n", file->offset);
    fprintf(backup_file, "%i\n", file->size);
    fprintf(backup_file, "%s\n", file->filename);
    fprintf(backup_file, "%s\n", file->contents);
    fclose(backup_file);
}

void restore_file_content_and_info(struct pb_fd *file)
{
    FILE *backup_file = fopen("file.backup", "r");
    fscanf(backup_file, "%i", &(file->fd));
    fscanf(backup_file, "%i", &(file->mode));
    fscanf(backup_file, "%li", &(file->offset));
    fscanf(backup_file, "%i", &(file->size));

    char fn[100];
    char *line = NULL;
    size_t len = 0;
    fscanf(backup_file, "%s", fn);
    len = strlen(fn);
    file->filename = malloc(sizeof(char) * len);
    file->filename = fn;


    getline(&line, &len, backup_file);

    getdelim(&line, &len, '\0', backup_file);
    file->contents = malloc(sizeof(char) * len);
    file->contents = line;

    fclose(backup_file);
}
