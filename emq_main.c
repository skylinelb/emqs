#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "emq_global.h"

int main(int argc, char **argv)
{
    int sock;
    int err_no;

    if(emqInit()) {
        printf("init emq env error\n");
        exit(0);
    }
    emqLoggingSetup(DEBUG, "./server.log");
    sock = emqNioServer(NULL, 8866);
    if(sock < 0) {
        emqDone();
        return -1; 
    }

    while(1)  {
       sleep(10);
    }
    close(sock);
    return 0;
}
