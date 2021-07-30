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
        //0x0000<xx>ba 0x00<yy>be00 0x<zz>bf0000 0xe8000000 0xfffffde6
        // lseek(0xaa, 0xbb, 0xcc);
        printf("%i\n", i);
        fflush(file);
        sleep(2);
    }
    fclose(file);
    return 0;
}