#ifndef _EMQ_TYPEBLOCK_INCLUDE  
#define _EMQ_TYPEBLOCK_INCLUDE  

#include "emq_global.h"

struct _emq_typeblock_request{
    int   sendIndex;
    int   receiveIndex;
    
    short type;
    int   function;
    int   operation;
    int   flag;
    int   bodyLength;
};

typedef struct _emq_typeblock_request EmqTypeBlockRequest;

struct _emq_typeblock_response{
    int   sendIndex;
    int   receiveIndex;

    short type;
    int   function;
    int   errType;
    long  errNo;
    char  errMsg[MAX_STR_LEN+1];
    int   flag;
    int   bodyLength;
};

typedef struct _emq_typeblock_response EmqTypeBlockResponse;

extern int emqTypeBlockRequestSend(EmqTypeBlockRequest *typeblock);
extern int emqTypeBlockResponseSend(EmqTypeBlockResponse *typeblock);
extern EmqTypeBlockRequest * emqTypeBlockRequestReceive();
extern EmqTypeBlockResponse* emqTypeBlockResponseReceive();
extern int emqTypeBlockResponseStreamLize(EmqTypeBlockResponse *tbRes, char *buff);
extern EmqTypeBlockResponse *emqTypeBlockResponseReStreamLize(char *buff);

#endif
