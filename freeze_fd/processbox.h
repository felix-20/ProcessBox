#include "pb_files.h"
#include "pb_memory.h"

pid_t getPidByName(char *name)
{
    //source: https://ofstack.com/C++/9293/linux-gets-pid-based-on-pid-process-name-and-pid-of-c.html
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[BUF_SIZE]; //The size is arbitrary, can hold the path of cmdline file
    char cur_name[50];       //The size is arbitrary, can hold to recognize the command line text
    char buf[BUF_SIZE];
    dir = opendir("/proc"); //Open the path to the
    if (NULL != dir)
    {
        while ((ptr = readdir(dir)) != NULL) //Loop reads each file/folder in the path
        {
            //If it reads "." or ".." Skip, and skip the folder name if it is not read
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
            {
                continue;
            }
            if (DT_DIR != ptr->d_type)
            {
                continue;
            }

            sprintf(filepath, "/proc/%s/status", ptr->d_name); //Generates the path to the file to be read
            fp = fopen(filepath, "r");                         //Open the file
            if (NULL != fp)
            {
                if (fgets(buf, BUF_SIZE - 1, fp) == NULL)
                {
                    fclose(fp);
                    continue;
                }
                sscanf(buf, "%*s %s", cur_name);

                //Print the name of the path (that is, the PID of the process) if the file content meets the requirement
                if (!strcmp(name, cur_name))
                {
                    //printf("PID:  %s", ptr->d_name);
                    return atoi(ptr->d_name);
                }
                fclose(fp);
            }
        }
        closedir(dir); //Shut down the path
    }
    printf("NOT FOUND");
    exit(1);
}
