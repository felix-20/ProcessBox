//
// Created by judith on 5/13/21.
//

#include <iostream>
#include <unistd.h>
using namespace std;

int main(int argc, char** argv)
{
    cout<<getpid()<<endl;
    sleep(5);
    int max = 99998;
    for(int i = 0; i < max; i++){
        sleep(1);
        cout<<"Hello World Object "<<i<<endl;
    }
    return 0;
}

