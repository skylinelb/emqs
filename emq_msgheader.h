#ifndef _EMQ_HEADER_INCLUDE
#define _EMQ_HEADER_INCLUDE

#include "emq_nio.h"

#define EMQ_MSG_HEADER_SIZE 26

typedef struct _emq_msg_header{
    int    sendIndex;
    int    receiveIndex; 
    int    version;          /* emq version */
    long   sequenceNum;      /* sequence num */
    short  splitFlag;        /* msg is or not by split */
    int    length;           /* the length of bolock plus msg body */
    char   check[4];         /* check bit map */
}EmqMsgHeader;

/* function */
extern int emqMsgHeaderSend(EmqNioConn *the_conn, EmqMsgHeader *header);
extern EmqMsgHeader *emqMsgHeaderReceive(EmqNioConn *the_conn);
extern EmqMsgHeader *emqMsgHeaderNew();
extern int emqMsgHeaderFree(EmqMsgHeader **header);
extern int  emqMsgHeaderStreamlize(EmqMsgHeader *header, char *buff);
extern EmqMsgHeader *emqMsgHeaderReStreamlize(char *buff);

#endif
