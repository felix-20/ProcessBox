#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    printf("PID: %ld\n", (long)getpid());
    volatile long long findme = 0xaaaadddd;
    int i;
    for (i = 0; i < 1000; ++i)
    { 
        printf("My counter: %d\n", i);
        sleep(2);
    }
    printf("I reached %i\n", i);
    return 0;
}