#include <stdlib.h>
#include <stdio.h>
#include "typeblock.h"
#include "emq_streamlize.h"

/* send type block request */
int emqTypeBlockRequestSend(EmqTypeBlockRequest *typeblock)
{

}

/* send type block response */
int emqTypeBlockResponseSend(EmqTypeBlockResponse *typeblock)
{

}

/* receive type block request */
EmqTypeBlockRequest * emqTypeBlockRequestReceive()
{

}

/* receive type block response */
EmqTypeBlockResponse * emqTypeBlockRequestResponse()
{

}

EmqTypeBlockRequest *emqTypeBlockRequestNew()
{
    EmqTypeBlockRequest *typeblock = NULL;
 
    typeblock = (EmqTypeBlockRequest *)malloc(sizeof(EmqTypeBlockRequest));
    if(typeblock == NULL) {
        printf("malloc error\n");
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

   len += int2buff(tbReq->sendIndex, buff);
   len += int2buff(tbReq->receiveIndex, buff+len);
   len += short2buff(tbReq->type, buff+len);
   len += int2buff(tbReq->function, buff+len);
   len += int2buff(tbReq->operation, buff+len);
   len += int2buff(tbReq->flag, buff+len);
   len += int2buff(tbReq->bodyLength, buff+len);

   return len;

}

EmqTypeBlockRequest *emqTypeBlockRequestReStreamLize(char *buff)
{
    EmqTypeBlockRequest *tbReq;

    if((tbReq = emqTypeBlockRequestNew()) == NULL)
        return NULL;

    tbReq->sendIndex = buff2int(buff);
    tbReq->receiveIndex = buff2int(buff+4);
    tbReq->type = buff2short(buff+8);
    tbReq->function = buff2int(buff+10);
    tbReq->operation = buff2int(buff+14);
    tbReq->flag = buff2int(buff+18);
    tbReq->bodyLength = buff2int(buff+22);

    return tbReq;
}

int emqTypeBlockResponseStreamLize(EmqTypeBlockResponse *tbRes, char *buff)
{
   int len = 0;

   len += int2buff(tbRes->sendIndex, buff);
   len += int2buff(tbRes->receiveIndex, buff+len);
   len += short2buff(tbRes->type, buff+len);
   len += int2buff(tbRes->function, buff+len);
   len += int2buff(tbRes->flag, buff+len);
   len += int2buff(tbRes->bodyLength, buff+len);

   return len;

}

EmqTypeBlockResponse *emqTypeBlockResponseReStreamLize(char *buff)
{
    EmqTypeBlockResponse *tbRes;

    if((tbRes = emqTypeBlockResponseNew()) == NULL)
        return NULL;

    tbRes->sendIndex = buff2int(buff);
    tbRes->receiveIndex = buff2int(buff+4);
    tbRes->type = buff2short(buff+8);
    tbRes->function = buff2int(buff+10);
    tbRes->flag = buff2int(buff+18);
    tbRes->bodyLength = buff2int(buff+22);

    return tbRes;
}
