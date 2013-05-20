#include "emq_global.h"

EmqNioThreadPool *_emqNioThreadPool; /* the nio thread pool */
EmqReqThreadPool *_emqReqThreadPool; /* the req process thread pool */
EmqSyncLink *_emqSyncLink;           /* the sync req link */
int _emqContinueFlag = 1;            /* running flag */
pthread_mutexattr_t _emq_mutex_attr;
pthread_condattr_t  _emq_cond_attr;

/* 
 * init global environment
 */
int emqInit()
{
    pthread_mutexattr_init(&_emq_mutex_attr);
    emqLoggingSetup(DEBUG, "./emqruning.log");

    applog(INFO, "init sync link");
    _emqSyncLink = emqSyncLinkInit(100);
    if(_emqSyncLink == NULL) {
        applog(INFO, "init sync link error");
        return -1;
    }
    applog(INFO, "init sync link finish");

    applog(INFO, "init nio thread pool");
    if(emqNioThreadPoolInit() != 0) {
        applog(INFO, "init nio thread pool error");
        return -1;
    }
    applog(INFO, "init nio thread pool finish");

    applog(INFO, "init req thread pool");
    if(emqReqThreadPoolInit(1) != 0) {
        applog(INFO, "init req thread pool error");
        return -1;
    }
    applog(INFO, "init req thread pool finish");

    return 0;
}

/*
 * free resource 
 */
void emqDone()
{
    applog(INFO, "free req thread pool begin");
    emqReqThreadPoolDestroy();
    applog(INFO, "free req thread pool finish");

    applog(INFO, "free nio thread pool begin");
    emqNioThreadPoolDestroy();
    applog(INFO, "free nio thread pool finish");

    pthread_mutexattr_destroy(&_emq_mutex_attr);
}
