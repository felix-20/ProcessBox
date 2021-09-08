#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    printf("PID: %ld\n", (long)getpid());
    FILE *file;
    file = fopen("out.txt", "w");
    int fd = fileno(file);
    printf("fd %d\n", fd);
    int i;
    for (i = 0; i < 1000; ++i)
    {
        fprintf(file, "%i\n", i);
        printf("%i\n", i);
        // refresh content of file
        fflush(file);
        sleep(2);
    }
    fclose(file);
    return 0;
}