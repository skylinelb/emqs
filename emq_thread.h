#ifndef _EMQ_THREAD_BASE_INCLUDE
#define _EMQ_THREAD_BASE_INCLUDE
#include <pthread.h>

int emqMutexInit(pthread_mutex_t * mutex);
int emqMutexLock(pthread_mutex_t * mutex);
int emqMutexUnlock(pthread_mutex_t * mutex);
int emqMutexDestroy(pthread_mutex_t * mutex);
int emqCondInit(pthread_cond_t * cond);
int emqCondWaitNew(pthread_cond_t * cond_var, pthread_mutex_t * cond_lock, \
                   long timeout, long * skip_time);
int emqCondWait(pthread_cond_t * cond_var, pthread_mutex_t * cond_lock, long timeout);
int emqCondSignal(pthread_cond_t * cond_var);
int emqCondBroadcast(pthread_cond_t * cond_var);
int emqCondDestroy(pthread_cond_t * cond);

#endif
