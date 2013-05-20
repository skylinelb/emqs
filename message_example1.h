#ifndef _EMQ_MESSAGE_EXAMPLE1_INCLUDE 
#define _EMQ_MESSAGE_EXAMPLE1_INCLUDE

#include "emq_message.h"

/* define operation no */
enum {
   MESSAGE_EXAMPLE1_OPERATION1 = 0,
   MESSAGE_EXAMPLE1_OPERATION2
   /* may be have other oper no */
};

/* define func_no */
enum {
    MESSAGE_EXAMPLE1_FUNCTION_REQ = 0,
    MESSAGE_EXAMPLE1_FUNCTION_REP = 1
};

/* define request data structure */
typedef struct _msg_example1_req{
    char id[50];
    char password[50];
}MsgExample1Req;

typedef struct _msg_example1_rep{
    int allow;
}MsgExample1Rep;

extern char *MsgExample1ReqStreamlize(MsgExample1Req *data, int *len);
extern MsgExample1Req *MsgExample1ReqRestreamlize(char *buff, int len);
extern char *MsgExample1RepStreamlize(MsgExample1Rep *data, int *len);
extern MsgExample1Rep *MsgExample1RepReStreamlize(char *buff, int len);

extern int MsgExampleReqCallbackFunc1(EmqMessage *reqMsg, EmqMessage *repMsg); 
extern int MsgExampleRepCallbackFunc1(EmqMessage *reqMsg, EmqMessage *repMsg); 
extern int MsgExampleReqCallbackFunc2(EmqMessage *reqMsg, EmqMessage *repMsg); 
extern int MsgExampleRepCallbackFunc2(EmqMessage *reqMsg, EmqMessage *repMsg); 

/* define process flag */
#define MSG_EXAMPLE1_OPERATION_DEFINE  \
            EMQ_GETFLAG_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ,MESSAGE_EXAMPLE1_OPERATION1, 0) \
            EMQ_GETFLAG_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ,MESSAGE_EXAMPLE1_OPERATION2, 0)

/* define streamlize func */
#define MSG_EXAMPL1_STREAMLIZE_REQ_DEFINE \
            EMQ_MESSAGE_STREAMLIZE_REQ_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ,\
                 MsgExample1ReqStreamlize,MsgExample1Req)

#define MSG_EXAMPL1_RESTREAMLIZE_REQ_DEFINE \ 
            EMQ_MESSAGE_RESTREAMLIZE_REQ_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ, \
                MsgExample1ReqRestreamlize,MsgExample1Req)

/* it don't need to define when the message haven't response */
#define MSG_EXAMPL1_STREAMLIZE_REP_DEFINE \
            EMQ_MESSAGE_STREAMLIZE_REP_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REP, \
                MsgExample1RepStreamlize,MsgExample1Rep)

#define MSG_EXAMPL1_RESTREAMLIZE_REP_DEFINE \
            EMQ_MESSAGE_RESTREAMLIZE_REP_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REP, \
                MsgExample1RepReStreamlize,MsgExample1Rep)

/* define callback func */
#define MSG_EXAMPL1_REQ_CALLBACKFUNC1_RDEFINE \
            EMQ_MESSAGE_REQ_CALLBACKFUNC_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ, \ 
                MESSAGE_EXAMPLE1_OPERATION1,MsgExampleReqCallbackFunc1)

#define MSG_EXAMPL1_REQ_CALLBACKFUNC2_RDEFINE \
            EMQ_MESSAGE_REQ_CALLBACKFUNC_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REQ, \ 
                MESSAGE_EXAMPLE1_OPERATION2,MsgExampleReqCallbackFunc2)

/* it need to define only when is async call */
#define MSG_EXAMPL1_REP_CALLBACKFUNC1_RDEFINE \
            EMQ_MESSAGE_REP_CALLBACKFUNC_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REP, \ 
                MESSAGE_EXAMPLE1_OPERATION1,MsgExampleRepCallbackFunc1)

#define MSG_EXAMPL1_REP_CALLBACKFUNC2_RDEFINE \
            EMQ_MESSAGE_REP_CALLBACKFUNC_DEFINE(1,MESSAGE_EXAMPLE1_FUNCTION_REP, \ 
                MESSAGE_EXAMPLE1_OPERATION2,MsgExampleRepCallbackFunc2)

#endif
