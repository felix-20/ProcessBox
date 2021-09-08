#include <sys/uio.h>

#define log printf

#define MAXSIZE 1 << 13
#define BUF_SIZE 1024

/*
ProcessBox File
*/
struct pb_file
{
    int fd;
    char *mode; // r/w/r+
    off_t offset;
    int size;
    char *filename; // with absolut path TODO: use relative paths
    char *contents;
};

/* -------- SAVE ---------- */

/*
Returns offset of file with (fd) of process with (pid)
*/
off_t get_offset(pid_t pid, int fd);

/*
Returns size of file with (fd) of process with (pid)
*/
int get_size(pid_t pid, int fd);

/*
Returns filename of file with (fd) of process with (pid)
*/
char *get_filename(pid_t pid, int fd);

/*
Returns access mode of file with (fd) of process with (pid)
*/
char *get_mode(pid_t pid, int fd);

/*
Set contents of (file)
!! filename must be set !! 
*/
void save_contents(struct pb_file *file);

/*
Set info of file with (fd) of process with (pid) in (file)
*/
void parse_file_info(pid_t pid, int fd, struct pb_file *file);

/*
Writes all fields of (file) into a backup file with (file_name)
*/
void save_file_info(struct pb_file *file, char *file_name);

/*
Finds the fd of a file of process with (pid) and save it's info into a file
This takes the first file in /proc that's not stdin, stdout or stderr
*/
void save_file(pid_t pid);

// /* -------- RESTORE ---------- */

/*
Creates file with (file)'s name and contents
*/
void restore_contents(struct pb_file *file);

/*
Makes process with (pid) open (file) with (file_path).
Ensure that the opened file has an identical fd of (file).
!! Only use this function, if the new process didn't open the file yet !!
!! For this we need GDB !!
*/
void restore_fd(pid_t pid, struct pb_file *file, char *file_path);

/*
Sets offset of process with (pid) to (file)'s offset.
!! For this we need GDB !!
*/
void restore_offset(pid_t pid, struct pb_file *file);

/*
Reads the saved file with (filename) and writes it into (file)
*/
void read_file_backup(struct pb_file *file, char *filename);

/*
Restores file that is saved in (backup_file) for process with (pid)
*/
void restore_file(pid_t pid, char *backup_file);
