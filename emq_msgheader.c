#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "emq_streamlize.h"
#include "emq_socket.h"
#include "emq_log.h"
#include "emq_msgheader.h"

/* send the header */
int emqMsgHeaderSend(EmqNioConn *the_conn, EmqMsgHeader *header)
{
    char buff[1024];
    int ret;

    memset(buff, 0x00, sizeof(buff));
    ret = emqMsgHeaderStreamlize(header, buff);
    if(ret != 0) {
        applog(EMERG, "func EmqMsgHeaderStreamlize error!");
        return -1; 
    }
    ret = socketSendDataToConn(the_conn, buff, EMQ_MSG_HEADER_SIZE);
/*
applog(INFO, "skyline test write socket ret is %d", ret);
*/
    if(ret != EMQ_MSG_HEADER_SIZE){
        applog(EMERG, "func socketWriteN error!");
        return -1;
    }

    return 0;
}

/* receiv the header */
EmqMsgHeader *emqMsgHeaderReceive(EmqNioConn *the_conn)
{
    char buff[EMQ_MSG_HEADER_SIZE+1];
    int ret;
    char buff1[1024];
   
    memset(buff, 0x00, sizeof(buff));
    ret = socketRecvDataFromConn(the_conn, buff, EMQ_MSG_HEADER_SIZE); 
    if(ret < 0 ) {
        applog(EMERG, "func socketRecvDataFromConn error %d", ret);
        return NULL;
    }
/*
applog(INFO, "skyline test socket read ret %d", ret);
*/
    if(ret != EMQ_MSG_HEADER_SIZE) {
        applog(EMERG, "func socketReadN error!");
        return NULL;
    }

    return emqMsgHeaderReStreamlize(buff);
}

/* create a header */
EmqMsgHeader *emqMsgHeaderNew()
{
    EmqMsgHeader *header = NULL;

    header = (EmqMsgHeader *)malloc(sizeof(EmqMsgHeader));
    if(header == NULL) {
        printf("malloc error\n");
        return NULL;
    }

    return header;
}

int emqMsgHeaderFree(EmqMsgHeader **header)
{
    if(header == NULL || *header == NULL)
        return -1;

    free(*header);
    header = NULL;

    return 0;
}

int  emqMsgHeaderStreamlize(EmqMsgHeader *header, char *buff)
{
   int  len = 0; 

   if(header == NULL || buff == NULL)
      return -1;

   len += int2buff(header->sendIndex, buff);
   len += int2buff(header->receiveIndex, buff+len);
   memcpy(buff+len,header->check,4);
   len+=4;
   len += int2buff(header->version, buff+len);
   len += long2buff(header->sequenceNum, buff+len);
   len += short2buff(header->splitFlag, buff+len);
   len += int2buff(header->length, buff+len);

   return 0;
}

EmqMsgHeader *emqMsgHeaderReStreamlize(char *buff)
{
    EmqMsgHeader *header;

    if((header = emqMsgHeaderNew()) == NULL)
        return NULL;

    header->sendIndex = buff2int(buff);
    header->receiveIndex = buff2int(buff+4);
    memcpy(header->check, buff+8, 4);
    header->version = buff2int(buff+12);
    header->sequenceNum = buff2int(buff+16);
    header->splitFlag = buff2short(buff+20);
    header->length = buff2int(buff+22);

    return header;
}
