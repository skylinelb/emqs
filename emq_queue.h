#ifndef _EMQ_QUEUE_INCLUDE
#define _EMQ_QUEUE_INCLUDE

#include "emq_message.h"

#define EMQ_PRIORITY 5

struct emq_queue {
   int size;                /* the size of queue */
   short type;              /* the queue type */
   EmqMessage **q_msgs[EMQ_PRIORITY];    /* the msg array of the queue  */
   int head[EMQ_PRIORITY]; 
   int tail[EMQ_PRIORITY]; 
   int length[EMQ_PRIORITY]; 
   int de_priority[EMQ_PRIORITY];
   int current_priority;
   int msgnum;

   pthread_mutex_t en_de_lock;
   pthread_cond_t  en_queue_cond[EMQ_PRIORITY];
   pthread_cond_t  de_queue_cond;  /*the cond of the queue*/
};
typedef struct emq_queue EmqQueue;

EmqQueue * emqQueueNew();
void emqQueueFree(EmqQueue *theQueue);
EmqQueue *emqQueueInit(int size, int type);
void emqQueueDestroy(EmqQueue *theQueue);
int emqMessagePutInQueue(EmqMessage *msg, EmqQueue *theQueue);
EmqMessage *emqMessageGetFromQueue(EmqQueue *theQueue, int mode);

extern EmqQueue *emqQueueInit(int size, int type);
extern void emqQueueDestroy(EmqQueue *theQueue);
extern int emqMessagePutInQueue(EmqMessage *msg, EmqQueue *theQueue);
extern EmqMessage *emqMessageGetFromQueue(EmqQueue *theQueue, int mode);

#endif
