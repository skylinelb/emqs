#include <stdlib.h>
#include <stdio.h>
#include "emq_typeblock.h"
#include "emq_streamlize.h"
#include "emq_socket.h"
#include "emq_nio.h"
#include "emq_global.h"

/* send type block request */
int emqTypeBlockRequestSend(EmqNioConn *the_conn, EmqTypeBlockRequest *tpReq)
{
    char buff[1024];
    int  ret, len; 

    memset(buff, 0x00, sizeof(buff));
    len = emqTypeBlockRequestStreamLize(tpReq, buff);
    if(len < 0) {
        applog(EMERG, "func emqTypeBlockRequestStreamLize error");
        return -1;
    }

    ret = socketSendDataToConn(the_conn, buff, len);
    if(ret != len ) {
        applog(EMERG, "func socketSendDataToConn error!");
        return -1;
    }

    return 0;
}

/* send type block response */
int emqTypeBlockResponseSend(EmqNioConn *the_conn, EmqTypeBlockResponse *tpRep)
{
    char buff[128 + MAX_STR_LEN];
    int ret, len;
  
    memset(buff, 0x00, sizeof(buff));
    len = emqTypeBlockResponseStreamLize(tpRep, buff);
    if(len < 0) {
        applog(EMERG, "func emqTypeBlockRequestStreamLize error");
        return -1;
    }

    ret = socketSendDataToConn(the_conn, buff, len);
    if(ret != len ) {
        applog(EMERG, "func socketSendDataToConn error!");
        return -1;
    }

    return 0;
}

/* receive type block request */
EmqTypeBlockResponse * emqTypeBlockResponseReceive(EmqNioConn *the_conn)
{
    char buff[1024];
    int func_no, ret;
    int err_len;
    EmqTypeBlockResponse *tpRep;

    tpRep = emqTypeBlockResponseNew();
    if(tpRep == NULL) {
        applog(EMERG, "func emqTypeBlockResponseNew error");
        return NULL;
    }

    memset(buff, 0x00, sizeof(buff));
    ret = socketRecvDataFromConn(the_conn, buff, 28);
    if(ret != 28 ) {
        applog(EMERG, "func socketRecvDataFromConn error!");
        emqTypeBlockResponseFree(&tpRep);
        return NULL;
    }
    tpRep->oper_no = buff2int(buff); 
    tpRep->priority = buff2int(buff+4); 
    tpRep->send_index = buff2int(buff+8); 
    tpRep->flag = buff2int(buff+12); 
    tpRep->body_len = buff2int(buff+16); 
    tpRep->err_type = buff2int(buff+20); 
    tpRep->err_no = buff2int(buff+24); 
    if(!(tpRep->err_type == 0 && tpRep->err_no == 0)) {
        memset(buff, 0x00, sizeof(buff));
        ret = socketRecvDataFromConn(the_conn, buff, 4);
        if(ret != 4 ) {
            applog(EMERG, "func socketSendDataToConn error!");
            emqTypeBlockResponseFree(&tpRep);
            return NULL;
        }
        err_len= buff2int(buff);
        if(err_len > 0 && err_len <= MAX_STR_LEN) {
            memset(buff, 0x00, sizeof(buff));
            ret = socketRecvDataFromConn(the_conn, buff, err_len);
            if(ret != err_len ) {
                applog(EMERG, "func socketRecvDataFromConn error!");
                emqTypeBlockResponseFree(&tpRep);
                return NULL;
            }
            memcpy(tpRep->err_msg, buff, err_len);
            tpRep->err_msg[MAX_STR_LEN] = '\0';
        }
    }

    return tpRep;
}

/* receive type block response */
EmqTypeBlockRequest * emqTypeBlockRequestReceive(EmqNioConn *the_conn)
{
    int ret;
    char buff[128];
    EmqTypeBlockRequest *tpReq = NULL;

    memset(buff, 0x00, sizeof(buff));
    ret = socketRecvDataFromConn(the_conn, buff, 20);
    if(ret != 20 ) {
        applog(EMERG, "func socketRecvDataFromConn error!");
        return NULL;
    }

    tpReq = emqTypeBlockRequestReStreamLize(buff); 
    if(tpReq == NULL) {
        applog(EMERG, "func emqTypeBlockRequestNew error!");
        return NULL;
    }

    return tpReq;
}

EmqTypeBlockRequest *emqTypeBlockRequestNew()
{
    EmqTypeBlockRequest *typeblock = NULL;
 
    typeblock = (EmqTypeBlockRequest *)malloc(sizeof(EmqTypeBlockRequest));
    if(typeblock == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }

    return typeblock; 
}

int emqTypeBlockRequestFree(EmqTypeBlockRequest **tbreq)
{
    if(tbreq == NULL || *tbreq == NULL)
        return -1;

    free(*tbreq);
    tbreq= NULL;
}

EmqTypeBlockResponse *emqTypeBlockResponseNew()
{
    EmqTypeBlockResponse *typeblock = NULL;
 
    typeblock = (EmqTypeBlockResponse *)malloc(sizeof(EmqTypeBlockResponse));
    if(typeblock == NULL) {
        printf("malloc error\n");
        return NULL;
    }

    return typeblock; 
}

int emqTypeBlockResponseFree(EmqTypeBlockResponse **tbres)
{
    if(tbres == NULL || *tbres == NULL)
        return -1;

    free(tbres);
    tbres = NULL;

    return 0; 
}

int emqTypeBlockRequestStreamLize(EmqTypeBlockRequest *tbReq, char *buff)
{
   int len = 0;
   if(tbReq == NULL || buff == NULL) {
        return -1;
   }

   len += int2buff(tbReq->func_no, buff+len);
   len += int2buff(tbReq->oper_no, buff+len);
   len += int2buff(tbReq->priority, buff+len);
   len += int2buff(tbReq->send_index, buff+len);
   len += int2buff(tbReq->flag, buff+len);
   len += int2buff(tbReq->body_len, buff+len);

   return len;
}

EmqTypeBlockRequest *emqTypeBlockRequestReStreamLize(char *buff)
{
    EmqTypeBlockRequest *tbReq;

    if(buff == NULL) {
        return NULL;
    }

    if((tbReq = emqTypeBlockRequestNew()) == NULL)
        return NULL;

    tbReq->oper_no = buff2int(buff);
    tbReq->priority = buff2int(buff+4);
    tbReq->send_index = buff2int(buff+8);
    tbReq->flag = buff2int(buff+12);
    tbReq->body_len = buff2int(buff+16);

    return tbReq;
}

int emqTypeBlockResponseStreamLize(EmqTypeBlockResponse *tbRes, char *buff)
{
   int len = 0;

   len += int2buff(tbRes->func_no, buff+len);
   len += int2buff(tbRes->oper_no, buff+len);
   len += int2buff(tbRes->priority, buff+len);
   len += int2buff(tbRes->send_index, buff+len);
   len += int2buff(tbRes->flag, buff+len);
   len += int2buff(tbRes->body_len, buff+len);
   len += int2buff(tbRes->err_type, buff+len);
   len += int2buff(tbRes->err_no, buff+len);
   if(!(tbRes->err_type == 0 && tbRes->err_no == 0)) {
       len += string2buff(tbRes->err_msg, buff+len);
   }

   return len;
}

EmqTypeBlockResponse *emqTypeBlockResponseReStreamLize(char *buff)
{
    EmqTypeBlockResponse *tbRes;
    int len = 0;

    if((tbRes = emqTypeBlockResponseNew()) == NULL)
        return NULL;

    len += 4;
    tbRes->err_type = buff2int(buff+len);
    len += 4;
    tbRes->err_no = buff2int(buff+len);
    len += 4;
    if(!(tbRes->err_type == 0 && tbRes->err_no == 0)) {
        len += buff2string(buff+len, tbRes->err_msg);
    }
    tbRes->flag = buff2int(buff+len);
    len += 4;
    tbRes->body_len = buff2int(buff+len);

    return tbRes;
}
