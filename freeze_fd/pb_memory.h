#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>


#define BUF_SIZE 1024

struct stack_space
{
    long start, end;
};

struct heap_space
{
    long start, end;
};

const int long_size = sizeof(long);

void getdata(pid_t pid, long addr, unsigned char *str, int len)
{

    int p = 0;
    unsigned char *laddr;
    int i, j;
    union u
    {
        long val;
        unsigned char chars[long_size];
    } data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j)
    {
        data.val = ptrace(PTRACE_PEEKDATA, pid,
                          addr + i * 4, NULL);
        if (data.val == -1)
        {
            printf("unable to read data %i\n", p);
            exit(1);
        }
        p += 1;
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        data.val = ptrace(PTRACE_PEEKDATA, pid,
                          addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}
void putdata(pid_t pid, long addr, unsigned char *str, int len)
{
    unsigned char *laddr;
    int i, j;
    union u
    {
        long val;
        unsigned char chars[long_size];
    } data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j)
    {
        memcpy(data.chars, laddr, long_size);
        if (ptrace(PTRACE_POKEDATA, pid,
                   addr + i * 4, data.val))
        {
            printf("unable to write data\n");
            exit(1);
        }
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0)
    {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, pid,
               addr + i * 4, data.val);
    }
}

struct stack_space get_stack_space(pid_t pid)
{
    FILE *fp;
    char line[2048], proc_path[50];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pid);
    fp = fopen(proc_path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct stack_space sp;
    do
    {
        if (strstr(line, "[stack]") != NULL)
        {
            return sp;
        }
        fscanf(fp, "%lx-%lx", &sp.start, &sp.end);
    } while (fgets(line, 2048, fp) != NULL);
    perror("Error finding stack");
    exit(1);
}

struct heap_space get_heap_space(pid_t pid)
{
    FILE *fp;
    char line[2048], proc_path[50];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/maps", pid);
    fp = fopen(proc_path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    struct heap_space hp;
    do
    {
        if (strstr(line, "[heap]") != NULL)
        {
            return hp;
        }
        fscanf(fp, "%lx-%lx", &hp.start, &hp.end);
    } while (fgets(line, 2048, fp) != NULL);
    perror("Error finding heap");
    exit(1);
}

pid_t getPidByName(char* name){
    //source: https://ofstack.com/C++/9293/linux-gets-pid-based-on-pid-process-name-and-pid-of-c.html
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[BUF_SIZE];//The size is arbitrary, can hold the path of cmdline file
    char cur_name[50];//The size is arbitrary, can hold to recognize the command line text
    char buf[BUF_SIZE];
    dir = opendir("/proc"); //Open the path to the
    if (NULL != dir)
    {
        while ((ptr = readdir(dir)) != NULL) //Loop reads each file/folder in the path
        {
            //If it reads "." or ".." Skip, and skip the folder name if it is not read
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {continue;}
            if (DT_DIR != ptr->d_type){continue;}

            sprintf(filepath, "/proc/%s/status", ptr->d_name);//Generates the path to the file to be read
            fp = fopen(filepath, "r");//Open the file
            if (NULL != fp)
            {
                if( fgets(buf, BUF_SIZE-1, fp)== NULL ){
                    fclose(fp);
                    continue;
                    }
                sscanf(buf, "%*s %s", cur_name);

                //Print the name of the path (that is, the PID of the process) if the file content meets the requirement
                if (!strcmp(name, cur_name)) {
                    //printf("PID:  %s", ptr->d_name);
                    return atoi(ptr->d_name);
                }
                fclose(fp);
            }

        }
        closedir(dir);//Shut down the path
    }
    printf("NOT FOUND");
    exit(1);
}
