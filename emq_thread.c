#include <errno.h>
#include "emq_thread.h"
#include "emq_log.h"

int emqMutexInit(pthread_mutex_t * mutex)
{
    int retval;

    retval = pthread_mutex_init(mutex, NULL);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_mutex_init() error %d", retval);
        return (-1);
    }
}

int emqMutexLock(pthread_mutex_t * mutex)
{
    int retval;

    retval = pthread_mutex_lock(mutex);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_mutex_lock() error %d", retval);
        return (-1);
    }
}

int emqMutexUnlock(pthread_mutex_t * mutex)
{
    int retval;

    retval = pthread_mutex_unlock(mutex);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_mutex_unlock() error %d", retval);
        return (-1);
    }
}

int emqMutexDestroy(pthread_mutex_t * mutex)
{
    int retval;

    if (mutex == NULL) 
        return (0);
    retval = pthread_mutex_destroy(mutex);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_mutex_destroy() error %d", retval);
        return (-1);
    }
}

int emqCondInit(pthread_cond_t * cond)
{
    int retval;

    retval = pthread_cond_init(cond, NULL);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_cond_init() error %d", retval);
        return (-1);
    }
}

int emqCondWaitNew(pthread_cond_t * cond_var, pthread_mutex_t * cond_lock, \
                   long timeout, long * skip_time)
{
    int retval;
    struct timespec time_st;
    struct timeval ct1, ct2;
    struct timezone zone;

    if (skip_time != NULL) {
        gettimeofday(&ct1, &zone);
    }
    if (timeout > 0) {
        memset(&time_st, 0, sizeof(struct timespec));
        time(&time_st.tv_sec);
        time_st.tv_sec += timeout;
        retval = pthread_cond_timedwait(cond_var, cond_lock, &time_st);
        switch (retval) {
        case 0:
            if (skip_time != NULL) {
                gettimeofday(&ct2, &zone);
                *skip_time = (ct2.tv_sec - ct1.tv_sec) * 1000000 + (ct2.tv_usec - ct1.tv_usec);
            }
            return (0);
        case ETIMEDOUT:
            if (skip_time != NULL) {
                *skip_time = timeout;
            }
            return (retval);
        default:
            if (skip_time != NULL) {
                *skip_time = 0;
            }
            applog(EMERG, "UNIX system call pthread_cond_timewait() error %d", retval);
            return (-1);
        }
    }
    else {
        retval = pthread_cond_wait(cond_var, cond_lock);
        switch (retval) {
        case 0:
            if (skip_time != NULL) {
                gettimeofday(&ct2, &zone);
                *skip_time = (ct2.tv_sec - ct1.tv_sec) * 1000000 + (ct2.tv_usec - ct1.tv_usec);
            }
            return (0);
        default:
            if (skip_time != NULL) {
                *skip_time = 0;
            }
            applog(EMERG, "UNIX system call pthread_cond_wait() error %d", retval);
            return (-1);
        }
    }
}

int emqCondWait(pthread_cond_t * cond_var, pthread_mutex_t * cond_lock, long timeout)
{
    int retval;
    struct timespec time_st;

    if (timeout > 0) {
        memset(&time_st, 0, sizeof(struct timespec));
        time(&time_st.tv_sec);
        time_st.tv_sec += timeout;
        retval = pthread_cond_timedwait(cond_var, cond_lock, &time_st);
        switch (retval) {
        case 0:
        case ETIMEDOUT:
            return (retval);
        default:
            applog(EMERG, "UNIX system call pthread_cond_timewait() error %d", retval);
            return (-1);
        }
    }
    else {
        retval = pthread_cond_wait(cond_var, cond_lock);
        switch (retval) {
        case 0:
            return (0);
        default:
            applog(EMERG, "UNIX system call pthread_cond_wait() error %d", retval);
            return (-1);
        }
    }
}

int emqCondSignal(pthread_cond_t * cond_var)
{
    int retval;

    retval = pthread_cond_signal(cond_var);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_cond_signal() error %d", retval);
        return (-1);
    }
}

int emqCondBroadcast(pthread_cond_t * cond_var)
{
    int retval;

    retval = pthread_cond_broadcast(cond_var);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_cond_broadcase() error %d", retval);
        return (-1);
    }
}

int emqCondDestroy(pthread_cond_t * cond)
{
    int retval;

    retval = pthread_cond_destroy(cond);
    switch (retval) {
    case 0:
        return (0);
    default:
        applog(EMERG, "UNIX system call pthread_cond_destroy() error %d", retval);
        return (-1);
    }
}
