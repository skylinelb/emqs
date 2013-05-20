#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "emq_socket.h"
#include "emq_global.h"
#include "emq_log.h"
#include "emq_nio.h"
#include "emq_queue.h"
#include "message_example1.h"

int main(int argc, char **argv)
{
    int sock;
    int err_no;
    EmqMessage *msg;
    EmqNioConn *the_conn = NULL; 
    int i;
    MsgExample1Req *req;
    MsgExample1Rep *rep;

    if(emqInit()) {
        printf("emq init env error\n");
        exit(-1);
    }
    emqLoggingSetup(DEBUG, "./client1.log");

    applog(INFO, "%s", "hello world");

    the_conn = (EmqNioConn *)emqNioConnectServer("127.0.0.1", 8866);
    if(the_conn == NULL) {
        applog(INFO,"connect to server error");
        exit(0);
    }

   for(i = 0 ; i < 50000; i++) {
       msg = (EmqMessage *)malloc(sizeof(EmqMessage));
       if(msg == NULL) {
          printf("mallo cerrlor \n");
          exit(-1);
       }
       memset(msg, 0x00, sizeof(EmqMessage));
/*
       msg->version = 1;
       msg->func_no = MESSAGE_EXAMPLE1_FUNCTION_REP;
       msg->oper_no = MESSAGE_EXAMPLE1_OPERATION1;
       rep = (void *)malloc(sizeof(MsgExample1Rep));
       rep->allow = 1;
       msg->data = (void *)rep;
       msg->priority = 0;
*/
       msg->version = 1;
       msg->func_no = MESSAGE_EXAMPLE1_FUNCTION_REQ;
       msg->oper_no = MESSAGE_EXAMPLE1_OPERATION1;
       msg->flag = MSG_FLAG_NEED_REP;
       req = (void *)malloc(sizeof(MsgExample1Req));
       if(req == NULL) {
           free(msg);
           continue;
       }
       sprintf(req->id ,"i:%d", i);
       req->id[40] = 0; 
       sprintf(req->password,"world");
       req->password[10] = 0;
       msg->data = (void *)req;
       msg->priority = 0;
       if(the_conn->sendQueue != NULL)
           emqMessagePutInQueue(msg, the_conn->sendQueue);
      else printf("the send queue is null");
       printf("the current productor is %d\n", i);
   }

    sleep(100);
    emqDone();
    return 0;
}
