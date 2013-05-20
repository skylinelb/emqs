#ifndef _EMQ_GLOBAL_INCLUDE
#define _EMQ_GLOBAL_INCLUDE
#include <pthread.h>
#include "emq_nio.h"
#include "emq_log.h"
#include "emq_queue.h"
#include "emq_socket.h"
#include "emq_reqproc.h"
#include "emq_synclink.h"

#define MAX_STR_LEN 1024
#define NETWORK_TIMEOUT 30 

extern pthread_mutexattr_t _emq_mutex_attr;
extern int _emqContinueFlag;

/* the nio part */
#define EMQ_NIO_THREADPOOL_THREAD_NUM_START 2  /* the default value of expand thread numbers */
#define EMQ_NIO_CONN_IDLE_NUM  2               /* default value of idle num */
#define EMQ_NIO_CONNS_PER_THREAD 2             /* num of conns per nio worker thread */
#define EMQ_NIO_THREAD_NUM 3                   /* num of nio worker thread */
extern EmqNioThreadPool *_emqNioThreadPool;

/* the req process part */
#define EMQ_REQ_THREADPOOL_CONNS_PER_THREAD 3   /* num of conns per req worker thread */
#define EMQ_REQ_THREADPOOL_THREAD_NUM_START 1   /* the default value of expand thread numbers */
extern EmqReqThreadPool *_emqReqThreadPool;
extern EmqSyncLink *_emqSyncLink;

/* the queue part */
#define EMQ_QUEUE_LENGTH 100                    /* length of queue */
#define EMQ_QUEUE_GET_MODE 2                    /* mode of get from the queue */                    

/* the log part */
extern void emqSetupLogging(int log_level, char* log_file_path);
extern void _emq_app_log(int level, int line, char *filename, const char *fmt, ...);

/* global */
extern int emqInit();
extern void emqDone();

#endif
