#include <sys/user.h>

#define log printf

#define MAXSIZE 1 << 13
#define BUF_SIZE 1024

/*
Virtual Memory Area Space
*/
struct vma_space
{
    long start, end;
};

static const int long_size = sizeof(long);

/*
Reads data from VMA of process with (pid) beginning at (addr) with (len) and wirtes in (data)
*/
void getdata(pid_t pid, long addr, unsigned char *data, int len);

/*
Writes (data) into VMA of process with (pid) beginning at (addr) with (len)
*/
void putdata(pid_t pid, long addr, unsigned char *data, int len);

/*
Returns vma space of stack for process with (pid)
*/
struct vma_space get_stack_space(pid_t pid);

/*
Returns vma space of heap for process with (pid)
*/
struct vma_space get_heap_space(pid_t pid);

    /*
Saves registers, stack and heap into binary files
*/
void save_vma(pid_t pid);

/*
Restores registers, stack and heap from binary files
*/
void restore_vma(pid_t pid);