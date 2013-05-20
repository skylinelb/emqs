#ifndef _EMQ_NIO_INCLUDE
#define _EMQ_NIO_INCLUDE

#include <sys/select.h>
#include <pthread.h>
#include "emq_queue.h"

#define EMQ_CONN_BUFF_LENGTH 4096

struct emq_nio_connection
{
    int fd;              /* fd for connection */
    char ipaddr[20];     /* peer ip addr */
    int  port;
    int used_flag;       /* is used or not */
    EmqQueue *recvQueue; /* receive queue */
    EmqQueue *sendQueue; /* send queue */
    int recv_index;       /* index of receive data */
    int send_index;       /* index of send data */
    int process_flag;    /* is or not processed by request worker */

    char *recvBuff;      /* recv buff */
    char *recviptr;      /* addr of recv buff data */
    char *recvoptr;      /* addr of recv buff put data */
    int  recvbuffsize;   /* size of recv buff */
    char *sendBuff;      /* send buff */
    char *sendiptr;      /* addr of send buff data */
    char *sendoptr;      /* addr of send buff put data */
    int  sendbuffsize;   /* size of recv buff */
    pthread_t dpid;      /* the destributed pthread id */
    struct emq_nio_connection *next;
};
typedef struct emq_nio_connection EmqNioConn;

struct emq_nio_thread_data
{
    pthread_t ptid;
    int index;
};
typedef struct emq_nio_thread_data EmqNioThreadData;

struct emq_nio_worker
{
    EmqNioConn     *connection;                 /* fd array */
    int             conn_num;                   /* connection count */
    EmqNioConn     *idle_conn;                  /* free connection  */
    EmqNioConn     *idle_conn_tail;             /* free connection oftail  */  
    int             idle_num;                   /* free connection number */
    int             expand_times;               /* auto expand times */
    pthread_t       ptid;                       /* the worker self's pid */
    pthread_mutex_t worker_mutex;               /* the lock for struct worker  */
    pthread_cond_t  worker_cond;                /* the cond for struct worker  */
};
typedef struct emq_nio_worker EmqNioWorker;

struct emq_nio_thread_pool
{
    int                thread_num;              /* thread number */
    int                valid_thread_num;        /* started thread number */
    EmqNioWorker     **tworker;                 /* the work thread for nio */
    int                curr_thread;             /* current thread for next fd */
    EmqNioThreadData **thread_data;
    int                conn_num;                /* all conn counts */
    pthread_mutex_t    pool_mutex ;             /* the lock for struct pool */ 
    pthread_mutex_t    xxxx_mutex ;             /* the lock for find already conn */ 
};
typedef struct emq_nio_thread_pool EmqNioThreadPool;

static int emqNioRecvMsg(EmqNioWorker *the_worker, EmqNioConn *the_conn);
static int emqNioSendMsg(EmqMessage *msg, EmqNioConn *the_conn);
static int emqNioThreadConnectionExpand(EmqNioWorker *the_worker, int size);
static int emqNioThreadConnectionInit(EmqNioWorker *the_worker);
static void emqNioThreadConnectionDestroy(EmqNioWorker *the_worker);
static int emqNioConnClean(EmqNioWorker *the_worker);
static void *emqNioWorkerEntrance(void *arg);
static int emqNioThreadPoolExpand(int size);
static void *emqNioServerEntrance(void *arg);
extern int emqNioServer(const char *bind_ipaddr, const int port);
extern EmqNioConn *emqInputConnNioWorker(int sockfd);
extern int emqNioDisconnectionConn(EmqNioConn *the_conn);
extern void emqNioThreadPoolDestroy();
extern int emqNioThreadPoolInit();
extern EmqNioConn *emqGetIdleConn(EmqNioWorker *the_worker);
extern EmqNioConn *emqNioConnectServer(const char *serv_addr, const int serv_port);
extern EmqNioConn *emqNioConnFind(char *ipaddr, int port);
#endif
