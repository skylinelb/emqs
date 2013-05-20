#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "emq_socket.h"
#include "emq_global.h"
#include "emq_log.h"
#include "emq_nio.h"
#include "emq_queue.h"

void *client_entrance(void *arg) 
{
    EmqNioConn *the_conn;
    EMQMessage *msg;
    int i;

    the_conn = (EmqNioConn *)emqNioConnectServer("127.0.0.1", 8866);
    if(the_conn == NULL) {
        applog(INFO,"connect to server error");
        exit(0);
    }

   for(i = 0 ; i < 10000; i++) {
       msg = (EMQMessage *)malloc(sizeof(EMQMessage));
       if(msg == NULL) {
          printf("mallo cerrlor \n");
          exit(-1);
       }
       msg->primBody = malloc(2048);
       if(msg->primBody == NULL) {
           free(msg);
           continue;
       }
       sprintf(msg->primBody ,"hello%dth\n", i);
       msg->priority = 0;
       if(the_conn->sendQueue != NULL)
           putTheEMQMessageInQueue(msg, the_conn->sendQueue);
      else printf("the send queue is null");
       printf("the current productor is %d\n", i);
   }

    sleep(20); 
    pthread_exit(NULL);

}

int main(int argc, char **argv)
{
    int i,num;
    pthread_t *ppid;
   
    if(argc != 2) {
        printf("usage:%s <num>\n");
        exit(0);
    }
    num = atoi(argv[1]);

    if(emqInit()) {
        printf("emq init env error\n");
        exit(-1);
    }
    emqLoggingSetup(DEBUG, "./client.log");
    ppid = (pthread_t *)malloc(sizeof(pthread_t)*num);
    for(i=0; i < num; i++) {
        pthread_create(&ppid[i], NULL, client_entrance, NULL);
    }

    for(i=0; i < num; i++) {
        pthread_join(ppid[i], NULL);
    }
   
    sleep(20);
    emqDone();
    return 0;
}
