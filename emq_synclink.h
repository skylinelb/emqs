#ifndef EMQ_SYNC_LINK_INCLUDE
#define EMQ_SYNC_LINK_INCLUDE

#include <pthread.h>
#include "emq_message.h"
#include "emq_nio.h"

struct emq_sync_link_node {
    char id[40];            /* id ip+port+send_index */
    int used_flag;          /* 0-isn't used 1-is used */
    int timeout_flag;       /* 0-not 1-yes when synccall timeout msg is destroyed */
    EmqMessage *rep_msg;    /* response msg */
    pthread_mutex_t mutex;  /* current node mutex */
    pthread_cond_t cond;    /* current node cond  */
};
typedef struct emq_sync_link_node EmqSyncLinkNode;

struct emq_sync_link {
    int size;                   /* sync link size */
    int used_num;               /* the num of be used node */
    pthread_mutex_t sy_mutex;   /* sync link mutex */
    EmqSyncLinkNode *node;      /* node array */
};
typedef struct emq_sync_link EmqSyncLink;

extern int emqSyncLinkSlot(EmqNioConn *the_conn, int index);
extern EmqSyncLink *emqSyncLinkInit(int num);
extern void emqSyncLinkDestroy(EmqSyncLink *the_link);
extern int emqSyncLinkEmptySlot();

int emqSyncLinkEmptySlotExpand();

#endif
