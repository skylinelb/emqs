#ifndef _EMQ_REQ_THREADPOOL_INCLUDE
#define _EMQ_REQ_THREADPOOL_INCLUDE

#include "emq_message.h"
#include <pthread.h>
#include "emq_nio.h"

#define EMQ_PRIORITY 5

struct emq_req_thread_data {
    pthread_t pid;
    pthread_mutex_t t_lock;
    pthread_cond_t t_cond;
    int index;
    EmqMessage *msg;
    int busy_flag;
    EmqNioConn *the_conn;
    struct emq_req_threadpool *the_threadpool;
};
typedef struct emq_req_thread_data EmqRegThreadData;

struct emq_req_threadpool {
    int size;           /* thead num */
    int high_water;     /* high water */
    int low_water;      /* low water */
    EmqRegThreadData *tp_data; /* worker thread data */
    int busy_num;       /* bysy worker num */
    pthread_t pid;      /* destribute pthread id */
    pthread_mutex_t tp_lock; /* the tp lock */
    pthread_cond_t tp_cond; /* the tp cond */
};
typedef struct emq_req_threadpool EmqReqThreadPool;

void processData(EmqMessage *msg, EmqNioConn *the_conn);
void *emqReqWorkerEntrance(void *arg);
void *emqReqDestributeEntrance(void *arg);
int   emqReqThreadPoolInit(int thread_num);
int   emqReqThreadPoolExpand(int thread_num);
int   emqReqThreadPoolDestroy();
int   emqSyncCall(char *ipaddr, int port, EmqMessage *req_msg, EmqMessage **rep_msg, int timeout);
int   emqAsyncCall(char *ipaddr, int port, EmqMessage *req_msg);

extern void* emqReqDestributeEntrance(void *arg);

#endif
