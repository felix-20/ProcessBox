#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#define BUF_SIZE 1024

int TIME = 5;

char* getPidByName(char* name){
    //source: https://ofstack.com/C++/9293/linux-gets-pid-based-on-pid-process-name-and-pid-of-c.html
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[50];//The size is arbitrary, can hold the path of cmdline file
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
                    return ptr->d_name;
                }
                fclose(fp);
            }

        }
        closedir(dir);//Shut down the path
    }
    return "NOT FOUND";
}

void waiting(void){
    for(int i = 0; i < TIME; i++){
        printf("#");
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

}

void sleepProcessByID(int pid, char* name){
    kill(pid, SIGSTOP);
    printf("%d (%s) is stopped now for %d seconds\n",pid,name, TIME);
    waiting();
    kill(pid, SIGCONT);
    printf("Done sleeping it, I woke it up again.\n");
}

void sleepProcessByName(char* name){
    printf("\nI will make %s fall asleep.\n", name);
    char* res = getPidByName(name);
    if(strcmp(res, "NOT FOUND") == 0){
        printf("Sorry, but I could not find your desired process. maybe there is a spelling mistake in it.\n");
        return;
    }else{
        int pid = atoi(res);
        sleepProcessByID(pid, name);
    }
}

void printUserManual(){
    printf("the following flags are allowed:\n"
           "-n to sleep process by name.\n"
           "-d to sleep process by pid.\n"
           "If you do not provide a flag, you can sleep a process by its pid.");
}

int main(int argc, char *argv[])
{
    if(argc == 2){
        if(strcmp(argv[1], "-n") == 0){
            char name[BUF_SIZE];
            printf("Hello. Please enter the name of the process you want to go to sleep: ");
            scanf("%s", name);
            printf("\nokay, I will stop process %s! \n", name);
            sleepProcessByName(name);
        }else{
            printUserManual();
        }
    }else if(argc > 2){
        printUserManual();
    }else {
        int pid;
        printf("Hello. Please enter a PID you would like to sleep for a bit: ");
        scanf("%d", &pid);
        printf("\nokay, I will stop process %d!\n", pid);
        sleepProcessByID(pid, "");
    }
    return 0;
}
