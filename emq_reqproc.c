#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emq_global.h"
#include "emq_log.h"
#include "emq_register.h"
#include "emq_reqproc.h"
#include "emq_thread.h"

void processData(EmqMessage *msg, EmqNioConn *the_conn)
{
    int ret;
    EmqMessage *repMsg;

    if(msg == NULL) {
       printf("the msg is null\n");
       return;
    }

applog(INFO, "process msg nio version:%d, func_no:%d, oper_no:%d,flag:%d", msg->version, msg->func_no, msg->oper_no, msg->flag);
    /* is request */
    if((msg->func_no % 2) == 0) {
         if(msg->flag & MSG_FLAG_NEED_REP) {
/*
             repMsg = (EmqMessage *)malloc(sizeof(EmqMessage));
             if(repMsg == NULL) {
                 applog(EMERG, "malloc memory error");
                 free(msg->data);
                 free(msg);
                 return;
             }
             memset(repMsg, 0x00, sizeof(EmqMessage));
             repMsg->version = msg->version;
             repMsg->func_no = msg->func_no + 1;
             repMsg->oper_no = msg->oper_no;
             repMsg->priority = msg->priority;
*/
             repMsg = emqMessageNew(msg->version,msg->func_no+1,msg->oper_no,0,msg->priority,0);
             if(repMsg == NULL) {
                 applog(EMERG, "fun emqMessageNew error");
                 emqMessageDestroy(&msg);
                 return;
             }
             ret = emqMessageReqCallback(msg, repMsg);
             /* put the response to queue */
             emqMessagePutInQueue(repMsg, the_conn->sendQueue);
         }else {
             applog(INFO, "not need response");
             ret = emqMessageReqCallback(msg , NULL);
         }
    }else { /* is async response */
         ret = emqMessageRepCallback(NULL, msg);
    }
    if(ret < 0) {
         applog(INFO, "call back func error");
    }

    emqMessageDestroy(&msg);

    return;
}

void *emqReqWorkerEntrance(void *arg)
{
    EmqRegThreadData *the_data = NULL;
    EmqReqThreadPool *the_threadpool;
    int index;

    pthread_detach(pthread_self());

    index = *(int *)arg;
    the_threadpool = _emqReqThreadPool;

    while(_emqContinueFlag) {
        the_data = &the_threadpool->tp_data[index];
        pthread_mutex_lock(&the_data->t_lock);
        while(the_data->busy_flag == 0 || the_data->msg == NULL) {
            pthread_cond_wait(&the_data->t_cond, &the_data->t_lock);
        }
  
        /*weake up */
/*
        pthread_mutex_unlock(&the_data->t_lock);
*/
        /*MessageProcessFun(the_data->msg);
        recv queue diffrient with send queue
        */
        processData(the_data->msg, the_data->the_conn);

/*
        pthread_mutex_lock(&the_data->t_lock);
*/
        the_data->msg = NULL;
        the_data->busy_flag = 0;
        pthread_mutex_unlock(&the_data->t_lock);

        pthread_mutex_lock(&the_threadpool->tp_lock);
        the_threadpool->busy_num--;
        pthread_cond_signal(&the_threadpool->tp_cond);
        pthread_mutex_unlock(&the_threadpool->tp_lock);
    }

    pthread_exit(0);
}

/*
EmqNioConn *getOneEMQConnFromConns()
{
    EmqNioConn *the_conn = NULL;
    EmqNioWorker *the_worker = NULL;

    pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
    the_worker = _emqNioThreadPool->tworker[_emqNioThreadPool->process_thread];
    _emqNioThreadPool->process_thread++;
    if(_emqNioThreadPool->process_thread >= _emqNioThreadPool->thread_num) {
        _emqNioThreadPool->process_thread = 0;
    }
    pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex); 

    pthread_mutex_lock(&the_worker->worker_mutex); 
    the_conn = the_worker->process_conn;
    if(the_conn == NULL) {
        pthread_mutex_unlock(&the_worker->worker_mutex); 
        return NULL;
    }
    the_worker->process_conn = the_worker->process_conn->next;
    if(the_worker->process_conn == NULL) {
        the_worker->process_conn = the_worker->connection;
    }
    pthread_mutex_unlock(&the_worker->worker_mutex); 

    return the_conn;
}
*/

void *emqReqDestributeEntrance(void *arg)
{
    EmqMessage *msg = NULL;
    EmqNioConn *the_conn = NULL;
    EmqReqThreadPool *the_threadpool;
    int i;
    
    pthread_detach(pthread_self());

    applog(INFO, "the req distribute thread start\n");
    while(_emqReqThreadPool == NULL) {
        sleep(1);
    }
    the_threadpool = _emqReqThreadPool;
    the_conn = (EmqNioConn *)arg;

    while(_emqContinueFlag) {
        /* the worker thread is enough? */
        if((_emqNioThreadPool->conn_num / the_threadpool->size + 1) \
            > EMQ_REQ_THREADPOOL_CONNS_PER_THREAD) {
            pthread_mutex_lock(&the_threadpool->tp_lock);

            if((_emqNioThreadPool->conn_num / the_threadpool->size + 1) \
                > EMQ_REQ_THREADPOOL_CONNS_PER_THREAD) {
                applog(INFO, "req process thread pool need to expand");
                emqReqThreadPoolExpand(EMQ_REQ_THREADPOOL_THREAD_NUM_START);
            }

            pthread_mutex_unlock(&the_threadpool->tp_lock);
        }

        msg = emqMessageGetFromQueue(the_conn->recvQueue, EMQ_QUEUE_GET_MODE); 
/*
        msg = getTheEmqMessageFromQueue(the_conn->recvQueue, 1); 
*/
        if(msg == NULL)  {
            continue;
        }

        pthread_mutex_lock(&the_threadpool->tp_lock); 
        while(1) {
            while(the_threadpool->busy_num >= the_threadpool->size) {
                applog(INFO, "req destributed thread wait here");
                pthread_cond_wait(&the_threadpool->tp_cond,&the_threadpool->tp_lock);
            }

            for(i = 0; i < the_threadpool->size; i++) {
                if(the_threadpool->tp_data[i].busy_flag == 0) {
                    break;
                }
            }

            if(i >= the_threadpool->size) {
/*
                pthread_mutex_unlock(&the_threadpool->tp_lock); 
*/
                continue;
            }

            pthread_mutex_lock(&the_threadpool->tp_data[i].t_lock); 
            if(the_threadpool->tp_data[i].busy_flag == 0) {
                break;
            }else {
                pthread_mutex_unlock(&the_threadpool->tp_data[i].t_lock); 
                continue;
            }
        }


        the_threadpool->busy_num++;
/*
        pthread_mutex_unlock(&the_threadpool->tp_lock); 

        pthread_mutex_lock(&the_threadpool->tp_data[i].t_lock); 
*/
        the_threadpool->tp_data[i].msg = msg;
        msg = NULL;
        the_threadpool->tp_data[i].the_conn = the_conn;
        the_threadpool->tp_data[i].busy_flag = 1;

        pthread_cond_signal(&the_threadpool->tp_data[i].t_cond);
        pthread_mutex_unlock(&the_threadpool->tp_data[i].t_lock); 
        pthread_mutex_unlock(&the_threadpool->tp_lock); 
    }

    pthread_exit(0);
}

int emqReqThreadPoolInit(int thread_num)
{
    int i;
    EmqReqThreadPool *the_threadpool;

    the_threadpool = (EmqReqThreadPool *)malloc(sizeof(EmqReqThreadPool));
    if(the_threadpool == NULL) {
        applog(EMERG, "malloc memory error");
        return errno != 0 ? errno : ENOMEM;
    }
    the_threadpool->size = thread_num; 
    the_threadpool->high_water = thread_num * 9/10; 
    the_threadpool->low_water = thread_num  * 2/10; 
    the_threadpool->size = thread_num; 
    _emqReqThreadPool = the_threadpool;

    /*init thread worker */
    the_threadpool->tp_data = (EmqRegThreadData *)malloc(sizeof(EmqRegThreadData) * thread_num);
    if(the_threadpool->tp_data == NULL) {
        applog(EMERG, "malloc memory error");
        return errno != 0 ? errno : ENOMEM;;
    }

    for(i = 0 ; i < thread_num; i++) {
        the_threadpool->tp_data[i].index = i;
        the_threadpool->tp_data[i].busy_flag = 0;
        the_threadpool->tp_data[i].msg = NULL;
        the_threadpool->tp_data[i].the_threadpool = the_threadpool;
        pthread_mutex_init(&the_threadpool->tp_data[i].t_lock,NULL);
        pthread_cond_init(&the_threadpool->tp_data[i].t_cond,NULL);

        pthread_create(&the_threadpool->tp_data[i].pid, NULL, emqReqWorkerEntrance, (void *)&the_threadpool->tp_data[i].index);
    }

    /* start destributed thread */
    the_threadpool->busy_num = 0;
    pthread_mutex_init(&the_threadpool->tp_lock, NULL);
    pthread_cond_init(&the_threadpool->tp_cond, NULL);

    return 0;
}

int emqReqThreadPoolExpand(int thread_num)
{
    int i;
    EmqReqThreadPool *the_threadpool;
    EmqRegThreadData *tp_data;
    int all_size;

    the_threadpool = _emqReqThreadPool;
/*
    pthread_mutex_lock(&the_threadpool->tp_lock);
*/
    all_size = thread_num + the_threadpool->size;

    /*init thread worker */
    tp_data = (EmqRegThreadData *)malloc(sizeof(EmqRegThreadData) * all_size);
    if(tp_data == NULL) {
        applog(EMERG, "malloc memory error");
/*
        pthread_mutex_unlock(&the_threadpool->tp_lock);
*/
        return errno != 0 ? errno : ENOMEM;;
    }
    memcpy(tp_data, the_threadpool->tp_data, sizeof(EmqRegThreadData) * the_threadpool->size);
    free(the_threadpool->tp_data);
    the_threadpool->tp_data = tp_data;
    for(i = the_threadpool->size ; i < all_size; i++) {
        the_threadpool->tp_data[i].index = i;
        the_threadpool->tp_data[i].busy_flag = 0;
        the_threadpool->tp_data[i].msg = NULL;
        the_threadpool->tp_data[i].the_threadpool = the_threadpool;
        pthread_mutex_init(&the_threadpool->tp_data[i].t_lock,NULL);
        pthread_cond_init(&the_threadpool->tp_data[i].t_cond,NULL);

        pthread_create(&the_threadpool->tp_data[i].pid, NULL, emqReqWorkerEntrance, (void *)(&the_threadpool->tp_data[i].index));
    }

    the_threadpool->size = all_size; 
/*
    pthread_mutex_unlock(&the_threadpool->tp_lock);
*/
    applog(INFO,"req process thread pool expand finish,pool size: %d", the_threadpool->size);

    return 0;
}

/*
 *destroy queue thread pool
 *free the resource of the queue thread pool
 */
int emqReqThreadPoolDestroy()
{
    int i;
    EmqReqThreadPool *the_threadpool;

    if(_emqReqThreadPool == NULL)
        return 0;
    the_threadpool = _emqReqThreadPool;

    for(i = 0; i < the_threadpool->size; i++) {
        /*queue woker thread cancel */
        if(pthread_cancel(the_threadpool->tp_data[i].pid) != 0) {
            applog(EMERG, "pthread cancel error pid: ", the_threadpool->tp_data[i].pid);
        }
    }

    free(the_threadpool->tp_data);
    the_threadpool->tp_data = NULL;

    /*free mutex */
    pthread_mutex_destroy(&the_threadpool->tp_lock);
    pthread_cond_destroy(&the_threadpool->tp_cond);

    return 0;
}

int emqSyncCall(char *ipaddr, int port, EmqMessage *req_msg, EmqMessage **rep_msg, int timeout)
{
    int slot;
    int ret;
    EmqSyncLink *the_link;
    EmqNioConn *the_conn = NULL;

    the_link = _emqSyncLink;

    /* init network conn */
    the_conn = emqNioConnFind(ipaddr, port);
    if(the_conn == NULL) {
        emqMutexLock(&_emqNioThreadPool->xxxx_mutex);
        the_conn = emqNioConnFind(ipaddr, port);
        if(the_conn == NULL) {
            the_conn = (EmqNioConn *)emqNioConnectServer(ipaddr, port);
            if(the_conn == NULL) {
                applog(INFO, "connect to server ip:%s,port:%d error", ipaddr, port);
                emqMutexUnlock(&_emqNioThreadPool->xxxx_mutex);
                return -1;
            }
        }
        emqMutexUnlock(&_emqNioThreadPool->xxxx_mutex);
    }

    /* init slot */
    slot = emqSyncLinkEmptySlot();
    if(slot < 0) {
        applog(INFO, "func emqSyncLinkEmptySlot error");
        return -1;
    }
    req_msg->slot = slot;
    req_msg->req_type = 0;
    req_msg->flag |= emqGetFlag(req_msg->version,req_msg->func_no, req_msg->oper_no);
    emqMessagePutInQueue(req_msg, the_conn->sendQueue);

    if(emqMutexLock(&the_link->node[slot].mutex)) {
        the_link->node[slot].timeout_flag = 1;
        applog(INFO, "func emqMutexLock error");
        return -1;
    } 
    if((ret = emqCondWait(&the_link->node[slot].cond, &the_link->node[slot].mutex, timeout)) != 0) {
        the_link->node[slot].timeout_flag = 1;
        emqMutexUnlock(&the_link->node[slot].mutex);
        applog(INFO, "func emqCondWait error");
        return ret;
    }

    /* weakup */
    *rep_msg = the_link->node[slot].rep_msg;
/*
    the_link->node[slot].rep_msg = NULL;
    the_link->node[slot].used_flag = 0;
    the_link->node[slot].id[0] = 0;
*/
    emqSyncLinkSlotClear(slot);
    emqMutexUnlock(&the_link->node[slot].mutex);

/*
    pthread_mutex_lock(&the_link->sy_mutex);
*/
    emqMutexLock(&the_link->sy_mutex);
    the_link->used_num--;
    emqMutexUnlock(&the_link->sy_mutex);

    if(*rep_msg == NULL) {
        return -1;
    }

    return  0;
}

int emqAsyncCall(char *ipaddr, int port, EmqMessage *req_msg)
{
    EmqNioConn *the_conn = NULL;

    /* init network conn */
    the_conn = emqNioConnFind(ipaddr, port);
    if(the_conn == NULL) {
        emqMutexLock(&_emqNioThreadPool->xxxx_mutex);
        the_conn = emqNioConnFind(ipaddr, port);
        if(the_conn == NULL) {
            the_conn = (EmqNioConn *)emqNioConnectServer(ipaddr, port);
            if(the_conn == NULL) {
                applog(INFO, "connect to server ip:%s,port:%d error", ipaddr, port);
                emqMutexUnlock(&_emqNioThreadPool->xxxx_mutex);
                return -1;
            }
        }
        emqMutexUnlock(&_emqNioThreadPool->xxxx_mutex);
    }

    req_msg->req_type = 1;
    req_msg->flag |= emqGetFlag(req_msg->version,req_msg->func_no, req_msg->oper_no);
    emqMessagePutInQueue(req_msg, the_conn->sendQueue);

    return 0;
}
