#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    printf("PID: %ld\n", (long)getpid());
    volatile long long findme = 0xaaaadddd;
    FILE *file;
    file = fopen("out.txt", "w");
    int fd = fileno(file);
    printf("fd %d\n", fd);
    fprintf(file, "HI\n");
    int i;
    for (i = 0; i < 1000; ++i)
    {
        fprintf(file, "%i\n", i);
        lseek(fd, 0, SEEK_END);
        printf("%i\n", i);
        fflush(file);
        sleep(2);
    }
    fclose(file);
    return 0;
}