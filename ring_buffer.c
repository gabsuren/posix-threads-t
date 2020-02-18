#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "ring_buffer.h"

typedef struct rbhandle_t_prv {
    uint8_t* buf;
    ecomp_t cmp;
    int entry_size;
    size_t head;
    size_t tail;
    size_t max;
    bool full;
    pthread_mutex_t queue_locker;     //mutex for all queue related operations
    pthread_cond_t  prod_locker;      //locks the producer when upper limit is riched, resumed by consumers signal
    pthread_cond_t  cons_locker;      //locks the consumer when buffer is empty, resumed by producers signal
    
} rbhandle_t_prv;

//Queue freeing
static void
free_queue(struct rbhandle_t_prv* queue) {
    pthread_cond_destroy(&queue->prod_locker);
    pthread_cond_destroy(&queue->cons_locker);
    pthread_mutex_destroy(&queue->queue_locker);
}

//Lock queue for short term manipulations (long suspends are using prod_locker and cons_locker)
static void
lock_queue(struct rbhandle_t_prv* queue) {
    pthread_mutex_lock(&queue->queue_locker);
}

//Unlock queue
static void
unlock_queue(struct rbhandle_t_prv* queue) {
    pthread_mutex_unlock(&queue->queue_locker);
}

static void
advance_pointer(rbhandle_t self)
{
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;

    if(h->full)
    {
        h->tail = (h->tail + 1) % h->max;
    }

    h->head = (h->head + 1) % h->max;
    h->full = (h->head == h->tail);
}

static bool
ring_buf_empty(rbhandle_t self)
{
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;
    
    return (!h->full && (h->head == h->tail));
}

static void
retreat_pointer(rbhandle_t self)
{
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;

    h->full = false;
    h->tail = (h->tail + 1) % h->max;
}

static void*
get_entry_ptr(rbhandle_t self, int num)
{
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;
    if(num > h->head)
        return 0;
    return h->buf + num * h->entry_size;
}
/**
 *   Create a new ring buffer instance
 */
rbhandle_t
RBufferCreate(int buf_size, ecomp_t comp, int entry_size)
{
    void* buf = malloc(buf_size * entry_size);
    //assert(buf);
    
    rbhandle_t_prv* r = malloc(sizeof(rbhandle_t_prv));
    r->cmp = comp;
    r->entry_size = entry_size;
    r->buf = buf;
    r->max = buf_size;
    r->head = 0;
    r->tail = 0;
    r->full = false;
    
    pthread_mutex_init(&r->queue_locker, NULL);
    pthread_cond_init(&r->prod_locker, NULL);
    pthread_cond_init(&r->cons_locker, NULL);
    
    return r;
}

/**
 *  Insert new entry to the ring buffer
 */
int
RBufferInsert(rbhandle_t self, void* e)
{
    //assert(self && e);
    
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;
    int r = -1;

    lock_queue(h); //enter critical section
    // Ensures that buffer is sorted, otherwise don't insert the new element
    if(ring_buf_empty(h) || 0 > h->cmp(get_entry_ptr(h, h->head), e)) {
        memcpy(get_entry_ptr(h, h->head), e, h->entry_size);
        if(h->full) {
            pthread_cond_wait(&h->prod_locker, &h->queue_locker); //suspend - queue is saturated (consumer signal will resume)
        }
        
        advance_pointer(h);
        r = 0;
    }
    pthread_cond_signal(&h->cons_locker);   //wake up any of suspended consumer - there is fresh data to read
    unlock_queue(h); //leave the critical section
    
    return r;
}

/**
*   Get an entry from the ring buffer
*    Non-blocking call. Returns immediatelly if buffer is empty
*/
int
RBufferGet(rbhandle_t self, void** e)
{
    //assert(self && e && self->buf);
    
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;
    
    lock_queue(h); //enter critical section
    if(ring_buf_empty(h)) {
        pthread_cond_wait(&h->cons_locker, &h->queue_locker); //suspend - queue is empty (producer signal will resume)
    }
    memcpy(*e, get_entry_ptr(h, h->tail), h->entry_size);
    retreat_pointer(h);
    pthread_cond_signal(&h->prod_locker); //wake up any of suspended producers - new free space is available
    unlock_queue(h); //leave the critical section
    return 0;
}

/**
*    Destroy the ring buffer
*
*/
void
RBufferDestroy(rbhandle_t self)
{
    assert(self);
    
    rbhandle_t_prv* h = (rbhandle_t_prv*)self;
    free_queue(h);
    free(h);
}
