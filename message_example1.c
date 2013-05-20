#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "message_example1.h"
#include "emq_log.h"

char *MsgExample1ReqStreamlize(MsgExample1Req *data, int *len)
{
    char buff[128]; 
    char *strTmp = NULL;
    int  data_len = 0;

    memset(buff, 0x00, sizeof(buff));
    *len = 0;

    data_len= string2buff(data->id, buff);
    data_len += string2buff(data->password,buff + data_len);

    strTmp = (char *)malloc(data_len);
    if(strTmp == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }

    memcpy(strTmp, buff, data_len);
    *len = data_len;
    return strTmp;
}

MsgExample1Req *MsgExample1ReqRestreamlize(char *buff, int len)
{
    MsgExample1Req *req;
    int len1;
    
    req = (MsgExample1Req *)malloc(sizeof(MsgExample1Req));
    if(req == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }

    len1 = buff2string(buff,req->id); 
    buff2string(buff+len1,req->password);

    return req; 
}

char *MsgExample1RepStreamlize(MsgExample1Rep *data, int *len)
{
    char buff[128]; 
    char *strTmp = NULL;

    memset(buff, 0x00, sizeof(buff));
    *len = 0;

    *len += int2buff(data->allow, buff);

    strTmp = (char *)malloc(*len);
    if(strTmp == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }

    memcpy(strTmp, buff, *len);
    return strTmp;
}

MsgExample1Rep *MsgExample1RepReStreamlize(char *buff, int len)
{
    MsgExample1Rep *rep;
    int len1;
    
    rep = (MsgExample1Rep *)malloc(sizeof(MsgExample1Rep));
    if(rep == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }

    rep->allow = buff2int(buff); 

    return rep; 
}

int MsgExampleReqCallbackFunc1(EmqMessage *reqMsg, EmqMessage *repMsg)
{
   MsgExample1Rep *rep = NULL;
   MsgExample1Req *req =(MsgExample1Req *)reqMsg->data; 
   
   if(repMsg == NULL) {
        applog(INFO, "the rep msg is null");
        return -1;
   }

   printf("skyline rpocess mes id:%s, passord:%s\n", req->id, req->password);

   rep = (MsgExample1Rep *)malloc(sizeof(MsgExample1Rep));
   if(rep == NULL) {
       applog(EMERG, "malloc memory error");
       free(reqMsg->data);
       free(reqMsg);
       return -1;
   }
   rep->allow = 1;
   repMsg->data =(void *)rep;
/*
   sleep(30);
*/

   return 0;
}

int MsgExampleRepCallbackFunc1(EmqMessage *reqMsg, EmqMessage *repMsg)
{
   MsgExample1Rep *rep =(MsgExample1Rep *)repMsg->data;

   applog(INFO, "skyline process mes allow:%d", rep->allow);
 
   free(repMsg->data);
   free(repMsg);

   return 0;
}

int MsgExampleReqCallbackFunc2(EmqMessage *reqMsg, EmqMessage *repMsg)
{

}

int MsgExampleRepCallbackFunc2(EmqMessage *reqMsg, EmqMessage *repMsg)
{

}
