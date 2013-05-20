#include <stdio.h>
#include <stdlib.h>
#include "emq_synclink.h"
#include "emq_global.h"
#include "emq_log.h"

EmqSyncLink *emqSyncLinkInit(int num)
{
    int i;
    EmqSyncLink *the_link;

    the_link = (EmqSyncLink *)malloc(sizeof(EmqSyncLink));
    if(the_link == NULL) {
        applog(EMERG, "malloc memory error");
        return NULL;
    }
    the_link->size = num;
    the_link->used_num = 0;
    pthread_mutex_init(&the_link->sy_mutex, NULL);
    the_link->node = (EmqSyncLinkNode *)malloc(sizeof(EmqSyncLinkNode) * num);
    if(the_link->node == NULL) {
        applog(EMERG, "malloc memory error");
        free(the_link);
        return NULL;
    }

    for(i = 0; i < num; i++) {
        the_link->node[i].id[0] = '\0';
        the_link->node[i].used_flag = 0;
        the_link->node[i].timeout_flag = 0;
        the_link->node[i].rep_msg = NULL;
        pthread_mutex_init(&the_link->node[i].mutex, NULL);
        pthread_cond_init(&the_link->node[i].cond, NULL);
    }

    return the_link;
}

void emqSyncLinkDestroy(EmqSyncLink *the_link)
{
    int i; 

    for(i = 0; i < the_link->size; i++) {
        if(the_link->node[i].rep_msg != NULL) {
            free(the_link->node[i].rep_msg);
        }
        pthread_mutex_destroy(&the_link->node[i].mutex);
        pthread_cond_destroy(&the_link->node[i].cond);
    }
    free(the_link->node);
    pthread_mutex_destroy(&the_link->sy_mutex);

    return;
}

int emqSyncLinkSlot(EmqNioConn *the_conn, int index)
{
    int i;
    char sid[40];
    EmqSyncLink *the_link;

    the_link = _emqSyncLink;
    memset(sid, 0x00, sizeof(sid));
    snprintf(sid, sizeof(sid)-1, "%s%05d%010d", the_conn->ipaddr, the_conn->port, index);
    
    for(i = 0; i < the_link->size; i++) {
       if(the_link->node[i].used_flag == 1 && \
           strcmp(the_link->node[i].id, sid) == 0) 
           break;
    }

    if(i >= the_link->size)
        return -1;
    return i;
}

int emqSyncLinkEmptySlotExpand()
{
    int i;
    EmqSyncLink *the_link;
    EmqSyncLinkNode *new_node;

    the_link = _emqSyncLink;
    new_node = (EmqSyncLinkNode *)malloc(sizeof(EmqSyncLinkNode)* (the_link->size+100));
    if(new_node == NULL) {
        applog(EMERG, "malloc memory error");
        return -1;
    }
    pthread_mutex_lock(&the_link->sy_mutex);
    memcpy(new_node, the_link->node, sizeof(EmqSyncLinkNode)*the_link->size);
    for(i= the_link->size; i < the_link->size+100; i ++) {
        new_node[i].id[0] = '\0';
        new_node[i].used_flag = 0;
        new_node[i].timeout_flag = 0;
        new_node[i].rep_msg = NULL;
        pthread_mutex_init(&new_node[i].mutex, NULL);
        pthread_cond_init(&new_node[i].cond, NULL);
    }
    free(the_link->node);
    the_link->node = new_node;
    the_link->size += 100;
    pthread_mutex_unlock(&the_link->sy_mutex);

    return 0;
}

int emqSyncLinkEmptySlot()
{
    int i;
    EmqSyncLink *the_link = NULL;
 
    the_link = _emqSyncLink;
    if(the_link->used_num >= the_link->size) {
        emqSyncLinkEmptySlotExpand();
    }

    for(i = 0; i < the_link->size; i++) {
       pthread_mutex_lock(&the_link->node[i].mutex);
       if(the_link->node[i].used_flag == 0) 
          break;
       pthread_mutex_unlock(&the_link->node[i].mutex);
    }

    if(i >= the_link->size)
        return -1;

    the_link->node[i].used_flag = 1;
    pthread_mutex_unlock(&the_link->node[i].mutex);

    pthread_mutex_lock(&the_link->sy_mutex);
    the_link->used_num++;
    pthread_mutex_unlock(&the_link->sy_mutex);
    return  i;
}

void emqSyncLinkNodeUpd(int slot, EmqNioConn *the_conn, int index)
{
    EmqSyncLink *the_link;

    the_link = _emqSyncLink;

    pthread_mutex_lock(&the_link->node[slot].mutex);
    snprintf(the_link->node[slot].id, sizeof(the_link->node[slot].id)-1, \
             "%s%05d%010d", the_conn->ipaddr, the_conn->port, index);
    pthread_mutex_unlock(&the_link->node[slot].mutex);
    
    return;
}

void emqSyncLinkSlotClear(int slot)
{
    EmqSyncLink *the_link = _emqSyncLink;

    the_link->node[slot].rep_msg = NULL;
    the_link->node[slot].used_flag = 0;
    the_link->node[slot].timeout_flag = 0;
    the_link->node[slot].id[0] = 0;

    return;
}
