#ifndef _EMQ_TYPEBLOCK_INCLUDE
#define _EMQ_TYPEBLOCK_INCLUDE

#include "emq_global.h"
#include "emq_nio.h"

struct _emq_typeblock_request{
    int   func_no;  /* function no */
    int   oper_no;  /* operation no */
    int   priority; /* msg priority */
    int   send_index; /* the client send msg index */
    int   flag;     /* process flag */
    int   body_len; /* body length */
};

typedef struct _emq_typeblock_request EmqTypeBlockRequest;

struct _emq_typeblock_response{
    int   func_no;                  /* function no */
    int   oper_no;                  /* operation no */
    int   priority;                 /* msg priority */
    int   send_index;               /* the client send msg index */
    int   err_type;                 /* err type */
    long  err_no;                   /* error no */
    char  err_msg[MAX_STR_LEN+1] ;  /* error message */
    int   flag;                     /* process flag */
    int   body_len;                 /* body length */
};

typedef struct _emq_typeblock_response EmqTypeBlockResponse;

extern EmqTypeBlockRequest *emqTypeBlockRequestNew();
extern int emqTypeBlockRequestFree(EmqTypeBlockRequest **tbreq);
extern int emqTypeBlockRequestSend(EmqNioConn *the_conn, EmqTypeBlockRequest *typeblock);
extern EmqTypeBlockRequest * emqTypeBlockRequestReceive(EmqNioConn *the_conn);
extern EmqTypeBlockRequest *emqTypeBlockRequestReStreamLize(char *buff);

extern EmqTypeBlockResponse *emqTypeBlockResponseNew();
extern int emqTypeBlockResponseFree(EmqTypeBlockResponse **tbres);
extern int emqTypeBlockResponseSend(EmqNioConn *the_conn, EmqTypeBlockResponse *typeblock);
extern EmqTypeBlockResponse* emqTypeBlockResponseReceive(EmqNioConn *the_conn);
extern int emqTypeBlockResponseStreamLize(EmqTypeBlockResponse *tbRes, char *buff);
extern EmqTypeBlockResponse *emqTypeBlockResponseReStreamLize(char *buff);

#endif
