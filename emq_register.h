#ifndef _EMQ_REGISTER_INCLUDE
#define _EMQ_REGISTER_INCLUDE

#include "emq_message.h"
#include "emq_log.h"

#define EMQ_GETFLAG_DECLEAR int emqGetFlag(int ver,int fun,int oper) {

#define EMQ_GETFLAG_DEFINE(VER,FUN,OPER,FLAG) if(ver == (VER) &&\
    fun == (FUN) && oper == (OPER)) return((FLAG));

#define EMQ_GETFLAG_END \
    applog(ISSUE, "not found get flag func,version:%d, function:%d, oper_no:%d", \
         ver, fun, oper); \
    return 0; \
}

/* define req streamlize func */
#define EMQ_MESSAGE_STREAMLIZE_REQ_DECLEAR  \
char *emqMessageReqStreamlize(EmqMessage *message ,int *len) \
{

#define EMQ_MESSAGE_STREAMLIZE_REQ_DEFINE(VER,FUN,FUNC,TYPE) \
   if(message->version == (VER) &&  message->func_no == (FUN)) {\
        char * (*funStreamlize)(TYPE *data, int *len); \
        funStreamlize = FUNC; \
        return funStreamlize((TYPE *)message->data, len); \
   }

#define EMQ_MESSAGE_STREAMLIZE_REQ_END \
    applog(ISSUE,"not found req strealize func,version:%d, func_no:%d", \
        message->version,message->func_no); \
    return NULL; \
}

/* define req restreamlize func */
#define EMQ_MESSAGE_RESTREAMLIZE_REQ_DECLEAR  \
void *emqMessageReqRestreamlize(char *buff ,int len, EmqMessage *message) \
{

#define EMQ_MESSAGE_RESTREAMLIZE_REQ_DEFINE(VER,FUN,FUNC,TYPE) \
   if(message->version == (VER) &&  message->func_no == (FUN)) {\
        TYPE * (*funRestreamlize)(char *buff, int len); \
        funRestreamlize = FUNC; \
        return (void *)(funRestreamlize(buff, len)); \
   }

#define EMQ_MESSAGE_RESTREAMLIZE_REQ_END \
    applog(ISSUE,"not found req restrealize func,version:%d, func_no:%d", \
        message->version,message->func_no); \
    return NULL; \
}

/* define rep streamlize func */
#define EMQ_MESSAGE_STREAMLIZE_REP_DECLEAR \
char *emqMessageRepStreamlize(EmqMessage *message ,int *len) \
{ 

#define EMQ_MESSAGE_STREAMLIZE_REP_DEFINE(VER,FUN,FUNC,TYPE) \
   if(message->version == (VER) &&  message->func_no == (FUN)) {\
        char * (*funStreamlize)(TYPE *data, int *len); \
        funStreamlize = FUNC; \
        return funStreamlize((TYPE *)message->data, len); \
   }

#define EMQ_MESSAGE_STREAMLIZE_REP_END \
    applog(ISSUE,"not fuound rep streamlize func,version:%d, function:%d", \
        message->version, message->func_no); \
    return NULL; \
}

/* define rep restreamlize func */
#define EMQ_MESSAGE_RESTREAMLIZE_REP_DECLEAR  \
void *emqMessageRepRestreamlize(char *buff ,int len, EmqMessage *message) \
{

#define EMQ_MESSAGE_RESTREAMLIZE_REP_DEFINE(VER,FUN,FUNC,TYPE) \
   if(message->version == (VER) &&  message->func_no == (FUN)) {\
        TYPE * (*funRestreamlize)(char *buff, int len); \
        funRestreamlize = FUNC; \
        return (void *)funRestreamlize(buff, len); \
   }

#define EMQ_MESSAGE_RESTREAMLIZE_REP_END \
    applog(ISSUE,"not found rep restreamlize func,version:%d, function:%d", \
        message->version, message->func_no); \
    return NULL; \
}

/* define req callback func */
#define EMQ_MESSAGE_REQ_CALLBACKFUNC_DECLEAR  \
int emqMessageReqCallback(EmqMessage *reqMsg, EmqMessage *repMsg) \
{ \
    int (*funCallback)(EmqMessage *reqMsg, EmqMessage *repMsg);

#define EMQ_MESSAGE_REQ_CALLBACKFUNC_DEFINE(VER,FUN,OPER,FUNC) \
   if(reqMsg->version == (VER) &&  reqMsg->func_no == (FUN) && \
      reqMsg->oper_no == (OPER)) {\
        funCallback = FUNC; \
        return funCallback(reqMsg, repMsg); \
   }

#define EMQ_MESSAGE_REQ_CALLBACKFUNC_END \
    applog(ISSUE,"not found req callback func,version:%d, function:%d,oper_no:%d", \
        reqMsg->version, reqMsg->func_no, reqMsg->oper_no); \
    return NULL; \
}

/* define rep callback func */
#define EMQ_MESSAGE_REP_CALLBACKFUNC_DECLEAR  \
int emqMessageRepCallback(EmqMessage *reqMsg, EmqMessage *repMsg) \
{ \
    int (*funCallback)(EmqMessage *reqMsg, EmqMessage *repMsg);

#define EMQ_MESSAGE_REP_CALLBACKFUNC_DEFINE(VER,FUN,OPER,FUNC) \
   if(repMsg->version == (VER) &&  repMsg->func_no == (FUN) && \
      repMsg->oper_no == (OPER)) {\
        funCallback = FUNC; \
        return funCallback(reqMsg, repMsg); \
   }

#define EMQ_MESSAGE_REP_CALLBACKFUNC_END \
    applog(ISSUE,"not found rep callback func,version:%d, function:%d,oper_no:%d", \
        repMsg->version, repMsg->func_no, repMsg->oper_no); \
    return NULL; \
}

extern int   emqGetFlag(int ver,int fun,int oper);
extern char *emqMessageReqStreamlize(EmqMessage *message ,int *len);
extern void *emqMessageReqRestreamlize(char *buff ,int len, EmqMessage *message);
extern char *emqMessageRepStreamlize(EmqMessage *message ,int *len);
extern void *emqMessageRepRestreamlize(char *buff ,int len, EmqMessage *message);
extern int   emqMessageReqCallback(EmqMessage *reqMsg, EmqMessage *repMsg);
extern int   emqMessageRepCallback(EmqMessage *reqMsg, EmqMessage *repMsg);

#endif
