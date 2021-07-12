#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    fpos_t position;
    if(argc < 2) fp = fopen("./test.txt", "w");
    else fp = fopen(argv[1],"w");
    fgetpos(fp, &position);
    printf("Hello World 1");
     printf ( "FILE: %p\n", fp);
    printf("PID: %ld\n", (long)getpid());
    volatile long long findme = 0xaaaadddd;
    int i;
    for (i = 0; i < 1000; ++i)
    { 
        printf("My counter: %d\n", i);
        fprintf(fp, "%d", i);
        fsetpos(fp, &position);
        fflush(fp);
 
        sleep(2);

    }
    printf("I reached %i\n", i);
    fclose(fp);

    // watch the file via
    //watch tail test.txt or file path you have chosen
    return 0;
}