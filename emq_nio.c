#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include <fcntl.h>
#include "emq_socket.h"
#include "emq_nio.h"
#include "emq_global.h"
#include "emq_msgheader.h"
#include "emq_message.h"
#include "emq_queue.h"
#include "emq_log.h"
#include "emq_typeblock.h"
#include "emq_register.h"
#include "emq_streamlize.h"
#include "emq_thread.h"

/*
 * recv date and create message
 *
 */
static int emqNioRecvMsg(EmqNioWorker *the_worker, EmqNioConn *the_conn)
{
     char *buff = NULL;
     char sfunc_no[5];
     int  func_no;
     int  slot;
     EmqMsgHeader *header = NULL;
     EmqTypeBlockRequest *tpReq = NULL; 
     EmqTypeBlockResponse *tpRep = NULL;
     EmqMessage *msg= NULL;
     int len, ret,iRet = 0;

     header = emqMsgHeaderReceive(the_conn);
     if(header == NULL) {
         applog(EMERG, "func EMQHeaerReceive error");
         return -1;
     }
     len = header->length;
     if(len < 0 || len > 99999) {
         applog(ISSUE, "invalied msg length");
         iRet = -1;
         goto errExit;
     }

     /* recv function no */
     memset(sfunc_no, 0x00, sizeof(sfunc_no));
     ret = socketRecvDataFromConn(the_conn, sfunc_no, 4);
     if(ret != 4) {
         if(ret != 0) {
             applog(EMERG, "func socketRecvDataFromConn error");
             iRet = -1;
         }
         goto errExit;
     }

     func_no = buff2int(sfunc_no);

     if(func_no < 0) {
         applog(ISSUE, "invalied function on"); 
         iRet = -1;
         goto errExit;
     }
     else if((func_no % 2) == 0) { /* is request */ 
         tpReq = emqTypeBlockRequestReceive(the_conn);
         if(tpReq == NULL) {
             applog(ISSUE, "func emqTypeBlockRequestReceive error!"); 
             iRet = -1;
             goto errExit;
         }
         tpReq->func_no = func_no;
     }
     else { /* is response */
         tpRep = emqTypeBlockResponseReceive(the_conn);
         if(tpRep == NULL) {
             applog(ISSUE, "func emqTypeBlockResponseReceive error!");
             iRet = -1;
             goto errExit;
         }
         tpRep->func_no = func_no;
     }

     len = (func_no % 2 == 0) ? tpReq->body_len:tpRep->body_len;
     buff = malloc(len+1);
     if(buff == NULL) {
         applog(EMERG, "malloc memory error, errno : %d", errno);
         iRet = -1;
         goto errExit;
     }
     memset(buff, 0x00, len+1);
     ret = socketRecvDataFromConn(the_conn, buff, len);
     if(ret != len) {
         if(ret != 0) { 
             applog(EMERG, "func socketRecvDataFromConn error");
             iRet = -1;
         }
         goto errExit;
     }

     /* create and process msg */
     if((func_no %2) == 0) {
         msg = emqMessageNew(header->version,tpReq->func_no,tpReq->oper_no, \
                   tpReq->flag,tpReq->priority, 0);
         if(msg == NULL) {
             applog(EMERG, "fun emqMessageNew error");
             iRet = -1;
             goto errExit;
         }
         msg->recv_index = tpReq->send_index;
         if(msg->priority < 0) msg->priority = 0;
         else if(msg->priority >= EMQ_PRIORITY) msg->priority = EMQ_PRIORITY-1;
         msg->data = emqMessageReqRestreamlize(buff, 0, msg);
         if(msg->data == NULL) {
             applog(INFO, "func EmqMessageReqRestreamlize error");
             iRet = -1;
             goto errExit;
         }

         emqMessagePutInQueue(msg, the_conn->recvQueue);
     }else {
         msg = emqMessageNew(header->version,tpRep->func_no,tpRep->oper_no, \
                   tpRep->flag,tpRep->priority, 0);
         msg->recv_index = tpRep->send_index;
         if(msg->priority < 0) msg->priority = 0;
         else if(msg->priority >= EMQ_PRIORITY) msg->priority = EMQ_PRIORITY-1;
         msg->data = emqMessageRepRestreamlize(buff, 0, msg);
         if(msg->data == NULL) {
             applog(INFO, "func EmqMessageRepResestreamlize error");
             iRet = -1;
             goto errExit;
         }

         /* is sync response */
         if((slot = emqSyncLinkSlot(the_conn,msg->recv_index)) >= 0) {
              pthread_mutex_lock(&_emqSyncLink->node[slot].mutex);
              if(_emqSyncLink->node[slot].timeout_flag == 1) {
                  free(msg->data);
                  free(msg);
                  emqSyncLinkSlotClear(slot);
              }else {
                  _emqSyncLink->node[slot].rep_msg = msg;
                  pthread_cond_signal(&_emqSyncLink->node[slot].cond);
              }
              pthread_mutex_unlock(&_emqSyncLink->node[slot].mutex);
         }else {
             /* is async response */
             emqMessagePutInQueue(msg, the_conn->recvQueue);
         }

     }

     the_conn->recv_index++;
applog(INFO, "process recv msg version:%d, func_no:%d, oper_no:%d, flag:%d,priority:%d,recv_index:%d", msg->version, msg->func_no, msg->oper_no, msg->flag,msg->priority, msg->recv_index);

applog(INFO, "skyline conn:%s,recv msg %d",the_conn->ipaddr, the_conn->recv_index);

errExit:
     if(header != NULL)
         emqMsgHeaderFree(&header);
     if(tpReq != NULL)
         emqTypeBlockRequestFree(&tpReq);
     if(tpRep != NULL)
         emqTypeBlockResponseFree(&tpRep);
     if(buff != NULL)
         free(buff);
     if(iRet == -1 && msg != NULL)
         emqMessageDestroy(&msg);

     return iRet;
}

static int emqNioSendMsg(EmqMessage *msg, EmqNioConn *the_conn)
{
    int ret,iRet = 0;
    char *strBody = NULL;
    int body_len;
    EmqMsgHeader *header = NULL;
    EmqTypeBlockRequest *tpReq = NULL;
    EmqTypeBlockResponse *tpRep = NULL;

    the_conn->send_index++;   
/*
applog(INFO, "skyline conn:%s,send msg %d",the_conn->ipaddr, the_conn->send_index);
*/
applog(INFO, "process send msg version:%d, func_no:%d, oper_no:%d, flag:%d, priority:%d", msg->version, msg->func_no, msg->oper_no, msg->flag, msg->priority);
    /* stream lize msg body */
    if((msg->func_no % 2 ) == 0) {
        strBody = emqMessageReqStreamlize(msg, &body_len);
    }else {
        strBody = emqMessageRepStreamlize(msg, &body_len);
    }
    if(strBody == NULL) {
        applog(INFO, "message streamlize error");
        return -1;
    }

    /* create header */
    header = emqMsgHeaderNew(); 
    header->version = msg->version;
    header->length = body_len;

    /* create type block */
    if((msg->func_no % 2) == 0) {
        tpReq= emqTypeBlockRequestNew();
        if(tpReq == NULL) {
            applog(EMERG, "malloc memory error");
            iRet = -1;
            goto errExit;
        }
        tpReq->func_no = msg->func_no;
        tpReq->oper_no = msg->oper_no;
        tpReq->priority = msg->priority;
        tpReq->send_index = the_conn->send_index;
        tpReq->flag = msg->flag;
        tpReq->body_len = body_len;
        if(msg->req_type == 0) {
            emqSyncLinkNodeUpd(msg->slot, the_conn, tpReq->send_index);
        }
    }else {
        tpRep = emqTypeBlockResponseNew();
        if( tpRep == NULL) {
            applog(EMERG, "malloc memory error");
            iRet = -1;
            goto errExit;
        }
        tpRep->func_no = msg->func_no;
        tpRep->oper_no = msg->oper_no;
        tpRep->priority = msg->priority;
        tpRep->send_index = the_conn->send_index;
        tpRep->flag = msg->flag;
        tpRep->body_len = body_len;
        tpRep->err_type = msg->err_type;
        tpRep->err_no = msg->err_no;
        memcpy(tpRep->err_msg, msg->err_msg, MAX_STR_LEN);
    }

    /* header send */
    ret = emqMsgHeaderSend(the_conn, header);
    if(ret < 0) {
        applog(ISSUE, "func emqMsgHeaderSend error");
        iRet = -1; 
        goto errExit;
    }

    /* type block send */
    if((msg->func_no %2) == 0) {
        ret = emqTypeBlockRequestSend(the_conn, tpReq);
        if(ret < 0) {
            applog(ISSUE, "func emqTypeBlockRequestSend error");
            iRet = -1; 
            goto errExit;
        }
    }else {
        ret =  emqTypeBlockResponseSend(the_conn, tpRep);
        if(ret < 0) {
            applog(ISSUE, "func emqTypeBlockResponseSend error");
            iRet = -1; 
            goto errExit;
        }
    }

    ret = socketSendDataToConn(the_conn, strBody, body_len);
    if(ret != body_len) {
        applog(EMERG, "func socketWriteN  error");
        iRet = -1; 
        goto errExit;
    }

       
errExit:
    if(header != NULL)
        emqMsgHeaderFree(&header);
    if(tpReq != NULL)
        emqTypeBlockRequestFree(&tpReq);
    if(tpRep != NULL)
        emqTypeBlockResponseFree(&tpRep);
    if(strBody != NULL)
        free(strBody);

    /*destroy msg*/
    if((msg != NULL) && \
      ((msg->func_no %2 == 0) && (msg->req_type == 1) || \
      (msg->func_no %2 != 0)))
        emqMessageDestroy(&msg);
    
    return iRet;
}

/* 
 * expand connection
 * size is the num of connections per once 
 * the_worker is the connection belong to
 */
static int emqNioThreadConnectionExpand(EmqNioWorker *the_worker, int size)
{
    EmqNioConn **ppConn, *pConn;
    int i,j;
 
    ppConn = (EmqNioConn **)malloc(sizeof(EmqNioConn *) * size);
    if(ppConn == NULL) {
        return errno != 0 ? errno : ENOMEM;
    }

    for(i = 0; i < size; i++) { 
        ppConn[i] = (EmqNioConn *)malloc(sizeof(EmqNioConn));
        if(ppConn[i] == NULL) {
            for(j = 0; j < i; j++) {
                free(ppConn[j]);
            }
            free(ppConn);
            return errno != 0 ? errno : ENOMEM;
        }

        ppConn[i]->fd = -1;
        ppConn[i]->used_flag = 0;
        ppConn[i]->next = NULL;
        ppConn[i]->recvQueue = emqQueueInit(100, 0);
        ppConn[i]->sendQueue = emqQueueInit(100, 1);
        /* may be have the other var */
        ppConn[i]->recv_index = 0;
        ppConn[i]->send_index = 0;
        ppConn[i]->recvBuff = (char *)malloc(EMQ_CONN_BUFF_LENGTH);
        if(ppConn[i]->recvBuff == NULL) {
            applog(EMERG, "malloc memory error");
            if(ppConn[i] == NULL) {
                for(j = 0; j < i; j++) {
                    free(ppConn[j]->recvBuff);
                    free(ppConn[j]->sendBuff);
                    free(ppConn[j]);
                }
                free(ppConn);
                return errno != 0 ? errno : ENOMEM;
            }
        }
        ppConn[i]->recvbuffsize = EMQ_CONN_BUFF_LENGTH;
        ppConn[i]->recviptr = ppConn[i]->recvoptr = ppConn[i]->recvBuff;

        ppConn[i]->sendBuff = (char *)malloc(EMQ_CONN_BUFF_LENGTH);
        if(ppConn[i]->sendBuff == NULL) {
            applog(EMERG, "malloc memory error");
            if(ppConn[i] == NULL) {
                for(j = 0; j < i; j++) {
                    free(ppConn[j]->recvBuff);
                    free(ppConn[j]->sendBuff);
                    free(ppConn[j]);
                }
                free(ppConn);
                return errno != 0 ? errno : ENOMEM;
            }
        }
        ppConn[i]->sendbuffsize = EMQ_CONN_BUFF_LENGTH;
        ppConn[i]->sendiptr = ppConn[i]->sendoptr = ppConn[i]->sendBuff;
        pthread_create(&ppConn[i]->dpid, NULL, emqReqDestributeEntrance, (void *)ppConn[i]);
    }   

    if(the_worker->idle_num == 0)
        the_worker->idle_conn = pConn = ppConn[0];
    else
        the_worker->idle_conn_tail->next = pConn = ppConn[0];

    for(j = 1; j < size; j++) {
        pConn->next = ppConn[j];
        pConn = pConn->next;
    }
    pConn->next = NULL;
    the_worker->idle_conn_tail = pConn;
    the_worker->idle_num += size;
    free(ppConn);
 
    return 0;
}

/*
 * get a idle conn
 *
 */
EmqNioConn *emqGetIdleConn(EmqNioWorker *the_worker)
{
     int ret;
     EmqNioConn * the_conn = NULL;

     applog(DEBUG, "choose the worker for conn idle_conn num %d", the_worker->idle_num);
     if(the_worker->idle_num <= 0) {
         applog(INFO, "need to expand idle worker");
         if( (ret = emqNioThreadConnectionExpand(the_worker, 2)) != 0){
            applog(EMERG, "emqNioThreadConnectionExpand excute error %d", ret);
            return NULL;
         }
         applog(INFO, "expand idle worker success");
     }

     the_conn = the_worker->idle_conn;
     the_worker->idle_conn = the_worker->idle_conn->next;
     the_conn->next = NULL;
     the_worker->idle_num--;
     if(the_worker->idle_conn == NULL)
        the_worker->idle_conn_tail = NULL;

     return the_conn;
}

static int emqNioThreadConnectionInit(EmqNioWorker *the_worker)
{
    int i,j;
    EmqNioConn **ppConn, *pConn;
    
    the_worker->connection = NULL;
    the_worker->conn_num = 0;

    applog(INFO, "idle conn init...");
    ppConn = (EmqNioConn **)malloc(sizeof(EmqNioConn *) * EMQ_NIO_CONN_IDLE_NUM);
    if(ppConn == NULL) {
        return errno != 0 ? errno : ENOMEM;
    }

    for(i = 0; i < EMQ_NIO_CONN_IDLE_NUM; i++) {
        ppConn[i] = (EmqNioConn *)malloc(sizeof(EmqNioConn));
        if(ppConn[i] == NULL) {
            for(j = 0; j < i; j++) {
                free(ppConn[j]->recvBuff);
                free(ppConn[j]->sendBuff);
                free(ppConn[j]);
            }
            free(ppConn);
            return errno != 0 ? errno : ENOMEM;
        }

        ppConn[i]->fd = -1;
        ppConn[i]->used_flag = 0;
        ppConn[i]->recvQueue = emqQueueInit(100, 0);
        ppConn[i]->sendQueue = emqQueueInit(100, 1);
        ppConn[i]->next = NULL;
        ppConn[i]->recv_index = 0;
        ppConn[i]->send_index = 0;
        ppConn[i]->recvBuff = (char *)malloc(EMQ_CONN_BUFF_LENGTH);
        if(ppConn[i]->recvBuff == NULL) {
            applog(EMERG, "malloc memory error");
            if(ppConn[i] == NULL) {
                for(j = 0; j < i; j++) {
                    free(ppConn[j]->recvBuff);
                    free(ppConn[j]->sendBuff);
                    free(ppConn[j]);
                }
                free(ppConn);
                return errno != 0 ? errno : ENOMEM;
            }
        }
        ppConn[i]->recvbuffsize = EMQ_CONN_BUFF_LENGTH;
        ppConn[i]->recviptr = ppConn[i]->recvoptr = ppConn[i]->recvBuff;

        ppConn[i]->sendBuff = (char *)malloc(EMQ_CONN_BUFF_LENGTH);
        if(ppConn[i]->sendBuff == NULL) {
            applog(EMERG, "malloc memory error");
            if(ppConn[i] == NULL) {
                for(j = 0; j < i; j++) {
                    free(ppConn[j]->recvBuff);
                    free(ppConn[j]->sendBuff);
                    free(ppConn[j]);
                }
                free(ppConn);
                return errno != 0 ? errno : ENOMEM;
            }
        }
        ppConn[i]->sendbuffsize = EMQ_CONN_BUFF_LENGTH;
        ppConn[i]->sendiptr = ppConn[i]->sendoptr = ppConn[i]->sendBuff;
        pthread_create(&ppConn[i]->dpid, NULL, emqReqDestributeEntrance, (void *)ppConn[i]);
        /* may be have the other var */
    }

    the_worker->idle_conn = pConn = ppConn[0];
    for(i = 1; i < EMQ_NIO_CONN_IDLE_NUM; i++) {
        pConn->next = ppConn[i];
        pConn = pConn->next;
    }
    pConn->next = NULL;
    the_worker->idle_conn_tail= pConn;
    the_worker->idle_num = EMQ_NIO_CONN_IDLE_NUM;
    free(ppConn);
    applog(INFO, "idle conn init finish %d",the_worker->idle_num);

    return 0;
}

static void emqNioThreadConnectionDestroy(EmqNioWorker *the_worker)
{
    EmqNioConn *pConn, *qConn;

    pConn = the_worker->idle_conn;

    while(pConn != NULL) {
        qConn = pConn->next;
        if(pthread_cancel(pConn->dpid) != 0) {
            applog(EMERG, "pthread cancel error pid: %d", pConn->dpid);
        }
        emqQueueDestroy(pConn->recvQueue);
        emqQueueDestroy(pConn->sendQueue);

        free(pConn->recvBuff);
        free(pConn->sendBuff);
        free(pConn);
        pConn = qConn;
    }

    the_worker->idle_conn_tail= NULL;
    the_worker->idle_conn= NULL;
    the_worker->idle_num = 0;
    return;
}

/*
 *clean the conn of close by peer 
 */
static int emqNioConnClean(EmqNioWorker *the_worker)
{
    EmqNioConn   *the_conn = NULL,*pConn;

    pthread_mutex_lock(&the_worker->worker_mutex);

    the_conn = the_worker->connection;
    while(the_conn != NULL) {
        if(the_conn->used_flag == 0){
applog(INFO, "skyline on socket is closed %d", the_conn->fd);
            pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
                _emqNioThreadPool->conn_num--;
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);

            the_worker->conn_num--;
/*
            if(the_worker->process_conn == the_conn) {
                 the_worker->process_conn = the_worker->process_conn->next;
                 if(the_worker->process_conn == NULL)
                     the_worker->process_conn = the_worker->connection;
            }
*/

            pConn = the_conn;
            the_conn = the_conn->next;
            the_worker->connection = the_conn;

            close(pConn->fd);
            pConn->fd = -1;
            pConn->next = NULL;
            pConn->recv_index = 0;
            pConn->send_index = 0;
            if(the_worker->idle_conn_tail != NULL) {
               the_worker->idle_conn_tail->next = pConn;
            }
            the_worker->idle_conn_tail = pConn;
 
            if(the_worker->idle_conn == NULL) {
                the_worker->idle_conn = pConn;
            }
            the_worker->idle_num++;
        }else
            break;
    }
  
    the_conn = the_worker->connection;
    while(the_conn != NULL && the_conn->next != NULL) {
        pConn = the_conn->next;
        if(pConn->used_flag == 0) {
applog(INFO, "skyline on socket is closed %d", the_conn->fd);
            pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
            _emqNioThreadPool->conn_num--;
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);

            the_worker->conn_num--;
/*
            if(the_worker->process_conn == the_conn) {
                the_worker->process_conn = the_worker->process_conn->next;
                if( the_worker->process_conn == NULL)
                     the_worker->process_conn = the_worker->connection;
            }
*/

            the_conn->next = pConn->next;
            pConn->next = NULL; 
            pConn->recv_index = 0;
            pConn->send_index = 0;
            close(pConn->fd);
            pConn->fd = -1;
            if(the_worker->idle_conn_tail != NULL) {
                the_worker->idle_conn_tail->next = pConn;
            }
            the_worker->idle_conn_tail = pConn;
            if(the_worker->idle_conn == NULL)
                the_worker->idle_conn = pConn;
            the_worker->idle_num++;
            continue;
        }

        the_conn = pConn;
    }

    pthread_mutex_unlock(&the_worker->worker_mutex);

    return 0;
}

static void *emqNioWorkerEntrance(void *arg)
{
    struct emq_nio_thread_data *thread_data;
    EmqNioWorker *the_worker = NULL;
    EmqNioConn   *the_conn = NULL;
    EmqMessage   *msg = NULL;
    fd_set readfds;
    fd_set writefds;
    fd_set errfds;
    int    nready;
    int    i;
    char   buff[1024];
    int    sockfd;
    int    maxfd = 0;
    int    nwrite;
    struct timeval timeout;

    pthread_detach(pthread_self());

    pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
    _emqNioThreadPool->valid_thread_num++;
    pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);

    thread_data = (struct emq_nio_thread_data *)arg;
    the_worker = (EmqNioWorker *)_emqNioThreadPool->tworker[thread_data->index];
    applog(INFO, "worker thread [%u] index[%d] started.", pthread_self(), thread_data->index);

    while(_emqContinueFlag) {

        pthread_mutex_lock(&the_worker->worker_mutex);
        while(the_worker->conn_num <= 0) {
            applog(INFO, "nio worker wait client join.");
            pthread_cond_wait(&the_worker->worker_cond,&the_worker->worker_mutex);
        }
        pthread_mutex_unlock(&the_worker->worker_mutex);

        /* find fds to be monitor by select */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        maxfd = 0;
        the_conn = the_worker->connection;
        while(the_conn != NULL) {
            if(the_conn->used_flag == 0)
                continue;

            if(maxfd < the_conn->fd)
                maxfd = the_conn->fd;
            FD_SET(the_conn->fd, &readfds);

            if(the_conn->sendQueue->msgnum > 0 || (the_conn->sendoptr - the_conn->sendiptr) > 0) {
                FD_SET(the_conn->fd, &writefds);
            }
            the_conn = the_conn->next;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 5;

       /*applog(INFO, "nio worker[%d] manage %d conn", thread_data->index, the_worker->conn_num); */
        nready = select(maxfd+1, &readfds, &writefds, NULL, &timeout);

        the_conn = the_worker->connection;
        while(the_conn != NULL) {
            sockfd = the_conn->fd;
            if(sockfd < 0 || the_conn->used_flag == 0) {
                the_conn = the_conn->next;
                continue;
            }

            /*process the readable fd */
            if(FD_ISSET(sockfd, &readfds) != 0 || (the_conn->recvoptr - the_conn->recviptr) > 0) {
/*
            if(FD_ISSET(sockfd, &readfds)) {
*/
                if(emqNioRecvMsg(the_worker, the_conn) != 0) {
                    applog(INFO, "recv data error");  
                }

                /*if(--nready <= 0)break; */
            }

            /*process the writeable fd */
            if(FD_ISSET(sockfd, &writefds)) {
                msg = emqMessageGetFromQueue(the_conn->sendQueue, EMQ_QUEUE_GET_MODE);
                if(msg == NULL) {
                    if((the_conn->sendoptr - the_conn->sendiptr) > 0) {
                        nwrite = send(the_conn->fd, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr, 0);
                        if(nwrite < 0 ) {
                           if (errno != EWOULDBLOCK) {
                               applog(INFO, "send data on sock error %d", errno);
                               emqNioDisconnectionConn(the_conn);
                           }
                        }else {
                            the_conn->sendiptr += nwrite;
                            if(the_conn->sendiptr == the_conn->sendoptr)
                                the_conn->sendiptr = the_conn->sendoptr = the_conn->sendBuff;
                        } 
                    } 
                    the_conn = the_conn->next;
                    continue;
                }
/*
applog(INFO, "skyline test send one msg");
*/
                if(emqNioSendMsg(msg, the_conn) != 0) {
                    applog(INFO, "send data error");
                }
                msg = NULL;

                /*if(--nready <= 0)break; */
             }
             the_conn = the_conn->next;
        }

        /* process the error fd
         the_conn = the_worker->connection;
         while(the_conn != NULL) {
        } 
        */

        /* clean the invalid conn */
        emqNioConnClean(the_worker);
    }

    free(thread_data);
}

int emqNioThreadPoolInit()
{
    int i,j;
    int result = 0;
    EmqNioThreadData *thread_data = NULL;

    applog(INFO, "nio thread pool init...");
    _emqNioThreadPool = NULL;
    _emqNioThreadPool = (EmqNioThreadPool *)malloc(sizeof(EmqNioThreadPool));
    if(_emqNioThreadPool == NULL) {
        /* log */
        return errno != 0 ? errno : ENOMEM;
    }
    memset(_emqNioThreadPool, 0x00, sizeof(EmqNioThreadPool));

    _emqNioThreadPool->thread_num = EMQ_NIO_THREAD_NUM;
    _emqNioThreadPool->tworker = (EmqNioWorker **)malloc(sizeof(EmqNioWorker *) * EMQ_NIO_THREAD_NUM);
    if(_emqNioThreadPool->tworker == NULL) {
        applog(EMERG, "malloc memory error");
        return errno != 0 ? errno : ENOMEM;
    }
    _emqNioThreadPool->thread_data = (EmqNioThreadData **)malloc(sizeof(EmqNioThreadData *) * EMQ_NIO_THREAD_NUM);
    if(_emqNioThreadPool->thread_data == NULL) {
        free(_emqNioThreadPool->tworker);
        _emqNioThreadPool->tworker = NULL;
        applog(EMERG, "malloc memory error");
        return errno != 0 ? errno : ENOMEM;
    }

    _emqNioThreadPool->curr_thread = 0;
    _emqNioThreadPool->conn_num = 0;
    pthread_mutex_init(&_emqNioThreadPool->pool_mutex, NULL);
    pthread_mutex_init(&_emqNioThreadPool->xxxx_mutex, NULL);

    for(i = 0; i < EMQ_NIO_THREAD_NUM; i++) {
        _emqNioThreadPool->tworker[i] = (EmqNioWorker *)malloc(sizeof(EmqNioWorker));
        if(_emqNioThreadPool->tworker[i] == NULL) {
            applog(EMERG, "malloc memory error");
            return errno != 0 ? errno : ENOMEM;
        }

        thread_data = (EmqNioThreadData *)malloc(sizeof(EmqNioThreadData));
        if(thread_data == NULL) {
            applog(EMERG, "malloc memory error");
            return errno != 0 ? errno : ENOMEM;
        }
        _emqNioThreadPool->thread_data[i] = thread_data;

        memset(_emqNioThreadPool->tworker[i], 0x00, sizeof(EmqNioWorker));
        _emqNioThreadPool->tworker[i]->conn_num = 0;
        pthread_mutex_init(&_emqNioThreadPool->tworker[i]->worker_mutex, NULL);
        pthread_cond_init(&_emqNioThreadPool->tworker[i]->worker_cond, NULL);


        emqNioThreadConnectionInit(_emqNioThreadPool->tworker[i]);

        memset(thread_data, 0x00, sizeof(struct emq_nio_thread_data));
        thread_data->index = i;
        if ((result=pthread_create(&thread_data->ptid, NULL,  emqNioWorkerEntrance, (void *)thread_data)) != 0)
        {
            for(j = 0; j < i;j++)
               free(_emqNioThreadPool->tworker[j]);
            free(_emqNioThreadPool->tworker);
            free(_emqNioThreadPool ); 
            _emqNioThreadPool = NULL;
            break;
        }
    }

    applog(INFO, "nio thread pool init finish.");
    return result;
}

void emqNioThreadPoolDestroy()
{
    int i;

    for(i = 0; i < _emqNioThreadPool->thread_num; i++) {
        if(pthread_cancel(_emqNioThreadPool->thread_data[i]->ptid) != 0) {
            applog(EMERG, "pthread cancel error pid: ", _emqNioThreadPool->thread_data[i]->ptid);
        }
        free(_emqNioThreadPool->thread_data[i]);
        _emqNioThreadPool->thread_data[i] = NULL;

        emqNioThreadConnectionDestroy(_emqNioThreadPool->tworker[i]);

        pthread_mutex_destroy(&_emqNioThreadPool->tworker[i]->worker_mutex);
        pthread_cond_destroy(&_emqNioThreadPool->tworker[i]->worker_cond);
        free(_emqNioThreadPool->tworker[i]);
        _emqNioThreadPool->tworker[i] = NULL;
    }

    free(_emqNioThreadPool->thread_data);
    _emqNioThreadPool->thread_data = NULL;

    pthread_mutex_destroy(&_emqNioThreadPool->pool_mutex);
    pthread_mutex_destroy(&_emqNioThreadPool->xxxx_mutex);
    free(_emqNioThreadPool);
    _emqNioThreadPool = NULL;

    return;
}

static int emqNioThreadPoolExpand(int size)
{
    struct emq_nio_thread_data * thread_data;
    EmqNioWorker **pTworkers;
    int            i,j;
    int            thread_num;
    pthread_t      ptid;


    applog(INFO, "nio thread pool expand...");
    pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);

    thread_num = _emqNioThreadPool->thread_num;
    pTworkers = (EmqNioWorker **)malloc(sizeof(EmqNioWorker) * (thread_num + size));
    if(pTworkers == NULL) {
        pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);
        return errno != 0 ? errno : ENOMEM;
    }
    memset(pTworkers, 0x00, sizeof(EmqNioWorker) * (thread_num + size));
    memcpy(pTworkers, _emqNioThreadPool->tworker, sizeof(EmqNioWorker) * thread_num);
    for(i = 0 ; i < size; i++) {
        pTworkers[thread_num + i] = (EmqNioWorker *)malloc(sizeof(EmqNioWorker));
        if(pTworkers[thread_num + i] == NULL) {
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);
            return errno != 0 ? errno : ENOMEM;
        }
        thread_data = (struct emq_nio_thread_data *)malloc(sizeof(struct emq_nio_thread_data));
        if(thread_data == NULL) {
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);
            return errno != 0 ? errno : ENOMEM;
        }
        memset(pTworkers[thread_num + i], 0x00, sizeof(EmqNioWorker));
        pTworkers[thread_num + i]->conn_num = 0;
        pthread_mutex_init(&pTworkers[thread_num + i]->worker_mutex, NULL);
        pthread_cond_init(&pTworkers[thread_num + i]->worker_cond, NULL);

        emqNioThreadConnectionInit(pTworkers[thread_num + i]);

        memset(thread_data, 0x00, sizeof(struct emq_nio_thread_data));
        thread_data->index = thread_num + i;
        if (pthread_create(&ptid, NULL,  emqNioWorkerEntrance, thread_data) != 0)
        {
            for(j = 0; j < i;j++)
               free(pTworkers[thread_num + j]);
            free(pTworkers);
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);
            return -1; 
        }
    }

    free(_emqNioThreadPool->tworker);
    _emqNioThreadPool->tworker = pTworkers;
    _emqNioThreadPool->thread_num += size;
    applog(INFO, "nio thread pool thread num is %d", _emqNioThreadPool->thread_num);
    pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);
    applog(INFO, "nio thread pool expand finish.");

    return 0;
}

EmqNioConn *emqInputConnNioWorker(int sockfd)
{
    int i, ret;
    EmqNioWorker *the_worker = NULL, *pWorker;
    EmqNioConn   *the_conn = NULL;

    while(1) {
        pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
        pWorker = _emqNioThreadPool->tworker[_emqNioThreadPool->curr_thread];
        if(pWorker->conn_num < EMQ_NIO_CONNS_PER_THREAD) {
            the_worker = pWorker;
            i = _emqNioThreadPool->curr_thread;
        }else{
            for(i = 0; i< _emqNioThreadPool->thread_num; i++) {
                pWorker = _emqNioThreadPool->tworker[i];
                if(pWorker->conn_num < EMQ_NIO_CONNS_PER_THREAD) {
                    the_worker = pWorker;
                    _emqNioThreadPool->curr_thread = i;
                    break;
                }
            }
        }
        if(the_worker != NULL)
              _emqNioThreadPool->conn_num++;
        pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);

        if(the_worker != NULL) {
            applog(INFO, "choose the nio worker[%d] %d %d", i, the_worker->conn_num, sockfd);
            pthread_mutex_lock(&the_worker->worker_mutex);
            if(pWorker->conn_num >= EMQ_NIO_CONNS_PER_THREAD) {
                pthread_mutex_unlock(&the_worker->worker_mutex);
                continue;
            }

            /* FD_SET(sockfd, &the_worker->readfds); */
            the_conn = emqGetIdleConn(the_worker);
            if(the_conn == NULL) {
                pthread_mutex_unlock(&the_worker->worker_mutex); 
                return NULL;
            }
            the_conn->used_flag = 1;
            the_conn->fd = sockfd;
            the_worker->conn_num++;
            socketGetIpAddr(sockfd, the_conn->ipaddr, &the_conn->port);
applog(INFO,"client ipaddr:%s,port:%d", the_conn->ipaddr, the_conn->port);
            if(the_worker->conn_num <= 1) {
                the_worker->connection = the_conn;
            }
            else {
                the_conn->next = the_worker->connection;
                the_worker->connection = the_conn;
            }

            pthread_cond_signal(&the_worker->worker_cond);
            pthread_mutex_unlock(&the_worker->worker_mutex);

            pthread_mutex_lock(&_emqNioThreadPool->pool_mutex);
            _emqNioThreadPool->curr_thread++;
            if(_emqNioThreadPool->curr_thread >= _emqNioThreadPool->thread_num)
                _emqNioThreadPool->curr_thread = 0;
            pthread_mutex_unlock(&_emqNioThreadPool->pool_mutex);

            break;
        }else{ /* need to expand pool */
            applog(INFO, "need to expand nio pool");
            if( (ret = emqNioThreadPoolExpand(1)) != 0) {
                applog(ISSUE,"expand nio pool error %d", ret);
                return NULL;
            }
            continue;
        }

        break;
    }

    return the_conn;
}

static void *emqNioServerEntrance(void *arg)
{
    int sock, val;
    int incomesock;
    struct sockaddr_in inaddr;
    socklen_t sockaddr_len;

    sock = *((int *)arg);
    free(arg);

    pthread_detach(pthread_self());
    sockaddr_len = sizeof(inaddr);

    while(_emqContinueFlag) {
        incomesock = accept(sock, (struct sockaddr *)&inaddr, &sockaddr_len);
printf("hello income sock %d ,errno: %d\n", incomesock, errno);
        if(incomesock < 0) {
            if(!(errno ==EINTR || errno == EAGAIN)) {
/*
                applog(EMERG,"socket accept error!, errno: %d, error info: %s", \
                errno, STRERROR(errno));
*/
            }
            continue;
        }
        applog(INFO, "one client in come %d", incomesock);

        val = fcntl(incomesock, F_GETFL, 0);
        fcntl(incomesock, F_SETFL, val | O_NONBLOCK);
        if(emqInputConnNioWorker(incomesock) == NULL ) {
            applog(EMERG, "func emqInputConnNioWorker error!");
            continue;
        }
    }

    /* close(socket); */
    return;
}

int emqNioServer(const char *bind_ipaddr, const int port)
{
    pthread_t ptid;
    int err_no;
    int sock, ret;
    int *pdata = NULL;

    pdata = (int *)malloc(sizeof(int));
    if(pdata == NULL) {
        applog(EMERG, "malloc memory error!");
        return -1;
    }
    sock = socketListenByAddrAndPort(bind_ipaddr, port, &err_no);
    if(sock < 0) {
        applog(EMERG, "func socketListenByAddrAndPort error!");
        free(pdata);
        return err_no;
    }
    *pdata = sock;
    ret = pthread_create(&ptid, NULL, emqNioServerEntrance, (void *)pdata);
    if(ret != 0) {
/*
        applog(EMERG, "pthread create error, errno: %d, error info: %s", \
            errno, STRERROR(errno));
*/
        return errno != 0 ? errno : EINVAL;
    }

    return 0;
}

int emqNioDisconnectionConn(EmqNioConn *the_conn)
{
    if(the_conn != NULL)
       the_conn->used_flag = 0;

    return 0; 
}

EmqNioConn *emqNioConnectServer(const char *serv_addr, const int serv_port)
{
    int sock,val;
    EmqNioConn *the_conn = NULL;

    sock = socketConnectToOther((char *)serv_addr, (int )serv_port);
    if(sock < 0) {
         applog(EMERG, "func socketConnectToOther error!");
         return NULL;
    }


    val = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, val | O_NONBLOCK);
    if((the_conn = emqInputConnNioWorker(sock)) == NULL ) {
        applog(EMERG, "func emqInputConnNioWorker error!");
        return NULL;
    }
    the_conn->port = serv_port;
    sprintf(the_conn->ipaddr, serv_addr);

    return the_conn;
}

EmqNioConn *emqNioConnFind(char *ipaddr, int port)
{
   int i;
   EmqNioThreadPool *the_pool; 
   EmqNioWorker *the_worker;
   EmqNioConn *the_conn;

   the_pool = _emqNioThreadPool;
   for(i = 0 ; i < the_pool->thread_num; i++) {
       the_conn = the_pool->tworker[i]->connection;
       while(the_conn != NULL) {
           if(the_conn->used_flag == 1 && \
              strcmp(the_conn->ipaddr, ipaddr) == 0 &&
              the_conn->port == port ) 
              break;
       }
  
       if(the_conn != NULL) return the_conn;
   }

   return NULL;
}
