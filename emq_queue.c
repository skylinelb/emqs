#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emq_queue.h"
#include "emq_global.h"
#include "emq_log.h"

EmqQueue * emqQueueNew()
{
    EmqQueue *theQueue;

    theQueue = (EmqQueue *)malloc(sizeof(EmqQueue)); 
    if(theQueue == NULL) {
        printf("malloc errlog!\n");
        return NULL;
    }

    memset(theQueue, 0x00, sizeof(EmqQueue));
    return theQueue;
}

void emqQueueFree(EmqQueue *theQueue)
{
    free(theQueue);
    return;
}

EmqQueue *emqQueueInit(int size, int type)
{
    EmqQueue *theQueue;
    int i, j;

    theQueue = emqQueueNew();
    if(theQueue == NULL) {
        return NULL;
    }
    theQueue->size = size;
    theQueue->type = type;
    theQueue->msgnum = 0;

    for(i = 0; i < EMQ_PRIORITY; i++) {
        theQueue->q_msgs[i] = (EmqMessage **)malloc(sizeof(EmqMessage *) * size);
        if(theQueue->q_msgs[i] == NULL) {
            applog(EMERG, "malloc memory error");
            for(j = 0; j < i; j ++) {
                free(theQueue->q_msgs[j]);
                theQueue->q_msgs[i] = NULL;
            }
            free(theQueue);
            return NULL;
        }

        for(j = 0; j < size; j++) {
            theQueue->q_msgs[i][j] = NULL;
        }
        theQueue->head[i] = 0;
        theQueue->tail[i] = 0;
        theQueue->length[i] = 0;
        theQueue->de_priority[i] = i;
        pthread_cond_init(&theQueue->en_queue_cond[i], NULL);
    }

    pthread_mutex_init(&theQueue->en_de_lock, NULL);
    pthread_cond_init(&theQueue->de_queue_cond, NULL);

    return theQueue;
}

void emqQueueDestroy(EmqQueue *theQueue)
{
    int i, j;
 
    if(theQueue == NULL )
       return;

    for(i = 0; i < EMQ_PRIORITY; i++) {
        pthread_cond_destroy(&theQueue->en_queue_cond[i]);
        for(j = 0; j < theQueue->size; j++) {
            if(theQueue->q_msgs[i][j] != NULL) {
                free(theQueue->q_msgs[i][j]);
                theQueue->q_msgs[i][j] = NULL;
            }
        }
        free(theQueue->q_msgs[i]);
        theQueue->q_msgs[i] = NULL;
    }

    pthread_mutex_destroy(&theQueue->en_de_lock);
    pthread_cond_destroy(&theQueue->de_queue_cond);

    emqQueueFree(theQueue);
}

int emqMessagePutInQueue(EmqMessage *msg, EmqQueue *theQueue)
{
    int priority;

    if(msg == NULL || theQueue == NULL)
        return -1;
    
    priority = msg->priority;
    if(priority < 0) {
        priority = msg->priority = 0;
    }else if(priority >= EMQ_PRIORITY) {
        priority = msg->priority = EMQ_PRIORITY - 1;
    }

    pthread_mutex_lock(&theQueue->en_de_lock);
    /*when queue full wait here */
    while(theQueue->length[priority] >= theQueue->size) {
/*
        applog(INFO, "hello en queue[%d] wait here",priority);
*/
        pthread_cond_wait(&theQueue->en_queue_cond[priority], &theQueue->en_de_lock);
    }

    /*weake up */
    theQueue->msgnum++;
    theQueue->q_msgs[priority][theQueue->tail[priority]] = msg;
    theQueue->tail[priority]++;
    if(theQueue->tail[priority] >= theQueue->size)
        theQueue->tail[priority] = 0;
    theQueue->length[priority]++;
    pthread_cond_signal(&theQueue->de_queue_cond);
    pthread_mutex_unlock(&theQueue->en_de_lock);

    return 0;
}

EmqMessage *emqMessageGetFromQueue(EmqQueue *theQueue, int mode)
{
    EmqMessage **the_msg = NULL;
    EmqMessage *msg = NULL;
    int i;

    pthread_mutex_lock(&theQueue->en_de_lock);
    while(1) {
        if(mode == 1) {/* the higher first */
            for(i = EMQ_PRIORITY - 1; i >= 0 ; i--) {
                if(theQueue->length[i] > 0) {
                    the_msg = theQueue->q_msgs[i]; 
                    theQueue->current_priority = i;
                    break;
                }
            }
        }else if(mode == 2) {/* polling */
            for(i = theQueue->current_priority; i < EMQ_PRIORITY; i++) {
                if(theQueue->length[i] > 0) {
                    the_msg = theQueue->q_msgs[i]; 
                    theQueue->current_priority = i;
                    break;
                }
            }

            if(the_msg == NULL) {
                for(i = 0; i < theQueue->current_priority; i++) {
                    if(theQueue->length[i] > 0) {
                        the_msg = theQueue->q_msgs[i]; 
                        theQueue->current_priority = i+1;
                        break;
                    }
                }
            }
        }else if(mode == 3) {
            theQueue->current_priority = EMQ_PRIORITY - 1;
            if(theQueue->length[theQueue->current_priority] > 0)
                the_msg = theQueue->q_msgs[theQueue->current_priority]; 

            for(i = EMQ_PRIORITY - 2; i >= 0 ; i--) {
                if(theQueue->length[i] > 0 && theQueue->de_priority[i] > theQueue->de_priority[theQueue->current_priority]) {
                    the_msg = theQueue->q_msgs[i]; 
                    theQueue->current_priority = i;
                    theQueue->de_priority[i] = i;
                }else {
                    theQueue->de_priority[i] += 2 ^ i;
                }
            }
        }

        if(the_msg == NULL) {
            /*applog(INFO, "de queue wait here\n");
            //pthread_cond_wait(&theQueue->de_queue_cond, &theQueue->en_de_lock);
            //applog(INFO, "the queue is empty\n");
            */
            pthread_mutex_unlock(&theQueue->en_de_lock);
            return NULL;
        }else 
            break;
    }

    theQueue->msgnum--;
    msg = the_msg[theQueue->head[theQueue->current_priority]];
    the_msg[theQueue->head[theQueue->current_priority]] = NULL;
    theQueue->length[theQueue->current_priority]--;
    theQueue->head[theQueue->current_priority]++;
    if(theQueue->head[theQueue->current_priority] >= theQueue->size)
        theQueue->head[theQueue->current_priority] = 0;

    pthread_cond_signal(&theQueue->en_queue_cond[theQueue->current_priority]);
    pthread_mutex_unlock(&theQueue->en_de_lock);

    return msg;
}
