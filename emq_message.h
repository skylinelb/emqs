#ifndef _EMQ_MESSAGE_INCLUDE
#define _EMQ_MESSAGE_INCLUDE

#define MAX_STR_LEN 1024

typedef struct _emq_message{ 
    int   version;                   /* the version of the current emq */
    int   func_no;                   /* the function of the message */
    int   oper_no;                   /* the opertion of the message */
    int   type;                      /* */
    int   flag;                      /* identify special process of this msg eg:need response */
    int   priority;                  /* priority of this msg */
    int   err_type;                  /* err type */
    long  err_no;                    /* error no */
    char  err_msg[MAX_STR_LEN+1];    /* error message */
    int   slot;                      /* sync req link slot */
    int   req_type;                  /* 0 is sync, 1 is async */
    int   recv_index;                /* the index of msg on network connection */
    void  *data;                     /* the data struct of application system */
    int   data_size;                 /* the data struct size */
}EmqMessage;

/* define macro process flag */
#define MSG_FLAG_NEED_REP     1
#define MSG_FLAG_SYS_FREE_MSG 2 

/* define macro wrong type */

/* define macro wrong no */

extern EmqMessage *emqMessageNew(int ver, int fun, int oper_no, int flag, int pri,int data_size);
extern void        emqMessageDestroy(EmqMessage **msg);
extern EmqMessage *emqMessageDup(EmqMessage *msg);
extern void        emqMessageSetError(EmqMessage *msg, const int err_type, \
                       const int err_no, const char *err_msg);
#endif
