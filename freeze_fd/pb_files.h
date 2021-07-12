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
    file->contents = malloc(sizeof(char) * file->size);
    fread(file->contents, sizeof(char), file->size, fptr);

out:
    free(buf);
}

// void write_chunk_fd(void *fptr, struct pb_fd *file)
// {
//     write_bit(fptr, &file->fd, sizeof(int));
//     write_bit(fptr, &file->mode, sizeof(int));
//     write_bit(fptr, &file->offset, sizeof(off_t));

//     int have_contents = !!(file->contents);
//     write_string(fptr, file->filename);
//     write_bit(fptr, &file->deleted, sizeof(int));
//     write_bit(fptr, &file->size, sizeof(int));
//     write_bit(fptr, &have_contents, sizeof(int));
//     if (file->contents)
// 	write_bit(fptr, file->contents, file->size);
// }

// /* -------- RESTORE ---------- */

// void read_chunk_fd(void *fptr)
// {
//     struct pb_fd fd;

//     read_bit(fptr, &fd.fd, sizeof(int));
//     read_bit(fptr, &fd.offset, sizeof(off_t));

//     read_chunk_fd_file(fptr, &fd);
// }

void restore_file(struct pb_fd *file){
    FILE *fptr = fopen(file->filename, "w");
    fprintf(fptr, file->contents);
    fclose(fptr);
}

void restore_fd(struct pb_fd *file)
{
    int ffd;
    ffd = open(file->filename, file->mode);

    if (ffd != file->fd)
    {
        dup2(ffd, file->fd);
        close(ffd);
    }
}

// void read_chunk_fd_file(void *fptr, struct pb_fd *fd)
// {
//     int have_contents;
//     fd->filename = read_string(fptr, NULL, 0);
//     read_bit(fptr, &fd->deleted, sizeof(int));
//     read_bit(fptr, &fd->size, sizeof(int));
//     read_bit(fptr, &have_contents, sizeof(int));
//     if (have_contents)
//     {
//         fd->contents = malloc(fd->size);
//         read_bit(fptr, fd->contents, fd->size);
//     }
//     else
//         fd->contents = NULL;

//     restore_fd_file(fd);
// }

void save_file_content_and_info(struct pb_fd *file, char* file_location){

    FILE *backup_file = fopen(file_location, "wb");
    fwrite(file->fd, 1, sizeof(file->fd), backup_file);
    fwrite(file->mode, 1, sizeof(file->mode), backup_file);
    fwrite(file->offset, 1, sizeof(file->offset), backup_file);
    fwrite(file->size, 1, sizeof(file->size), backup_file);
    fwrite(file->filename, 1, sizeof(file->filename), backup_file);
    fwrite(file->contents, 1, sizeof(file->contents), backup_file);
    fclose(backup_file);
}

void restore_file_content_and_info(struct pb_fd *file, char* backup_file_location){
    FILE *backup_file = fopen(backup_file_location, "r");
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;
    if (!backup_file)   {
        fprintf(stderr, "Error opening file '%s'\n", backup_file_location);
        return EXIT_FAILURE;
    }
    getline(&line_buf, &line_buf_size, fp);
    file->fd=strol(line_buf);
    getline(&line_buf, &line_buf_size, fp);
    file->mode=strol(line_buf);
    getline(&line_buf, &line_buf_size, fp);
    file->offset=stroll(line_buf);
    getline(&line_buf, &line_buf_size, fp);
    file->size=strol(line_buf);
    //set line_size to get into while loop
    line_size=getline(&line_buf, &line_buf_size, fp);
    file->filename=line_buf;

    FILE *content_stream;
    char *buf;
    size_t len;
    content_stream = open_memstream(&buf, &len);

     while (line_size >= 0)
  {
    line_size = getline(&line_buf, &line_buf_size, fp);
    fprintf(content_stream, line_buf);
  }
  fclose(content_stream);
  fclose(backup_file);
  file->contents=buf;
  free(buf);
  free(line_buf);
  restore_file(file);
  restore_fd(file);

}

/*
struct pb_fd
{
    int fd;
    int mode;
    off_t offset;
    int size;
    char *filename;
    char *contents;
};
*/

}