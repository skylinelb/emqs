#include <stdlib.h>
#include <stdio.h>
#include "emq_message.h"
#include "emq_log.h"

EmqMessage *emqMessageNew(int ver, int fun, int oper_no, int flag, int pri,int data_size)
{
    EmqMessage *message = NULL;

    message = (EmqMessage *)malloc(sizeof(EmqMessage));
    if(message == NULL) {
        applog(EMERG, "malloc memory error\n");
        return NULL;
    }

    memset(message, 0x00, sizeof(EmqMessage));
    message->version = ver; 
    message->func_no = fun;
    message->oper_no = oper_no;
    message->flag = flag;
    message->priority = pri;
 
    message->data = NULL; 
    if(data_size > 0) {
        message->data_size = data_size;
        message->data = (void *)malloc(data_size);
        if(message->data == NULL) {
            free(message);
            applog(EMERG, "malloc memory error\n");
            return NULL;
        }
        memset(message->data, 0x00, data_size);
    }

    return message;
}

void emqMessageDestroy(EmqMessage **msg)
{
    EmqMessage *pmsg;

    pmsg = *msg;
    if(pmsg->data != NULL) {
        free(pmsg->data);
        pmsg->data = NULL;
    }

    free(pmsg);
    msg = NULL;
}

EmqMessage *emqMessageDup(EmqMessage *msg)
{
    EmqMessage *new_msg;

    new_msg = emqMessageNew(0,0,0,0,0,msg->data_size);
    if(new_msg == NULL) {
        applog(INFO, "fun emqMessageNew error");
        return NULL;
    }
    memcpy(new_msg, msg, sizeof(EmqMessage));
    memcpy(new_msg->data, msg->data, msg->data_size);

    return new_msg;
}

void emqMessageSetError(EmqMessage *msg, const int err_type, \
                       const int err_no, const char *err_msg)
{
    int len;

    if(msg == NULL)
        return;

    msg->err_type = err_type;
    msg->err_no = err_no;
    msg->err_msg[0] = '\0';
    if(err_msg != NULL) {
        len = strlen(err_msg) < MAX_STR_LEN ? strlen(err_msg):MAX_STR_LEN;
        memcpy(msg->err_msg, err_msg, len);
        msg->err_msg[len] = '\0';
    }

    return;
}
