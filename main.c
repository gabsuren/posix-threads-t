/*
Ring buffer procucer-consumer task solution.
The application must be stopped by pressing Ctrl+C
To build release run:
    make
or:
    make rel
To build non-stripped version with debug info and additional tracings run:
    make deb
To create source code and Makefile archive run:
    make tgz
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include "ring_buffer.h"

#ifdef ENABLE_TRACE
    #define TRACE                fprintf
#else
    #define TRACE(...)
#endif

int create_ring_buffer();
void* producer(void*);
void* consumer(void*);

rbhandle_t rb;
int run = 1;

int
comparator(void *e1, void *e2)
{
    int* l = (int*)e1;
    int* r = (int*)e2;
    if (*l > *r)
        return 1;
    else if (*l < *r)
        return -1;
    return 0;
}

void
ctrl_c(int sig) {  //program runs infinitely, can be stopped using Ctrl+C
    run = 0;
}

int main(int argc, const char* argv[])
{
    pthread_t prod_th, cons_th;
    
    fprintf(stderr, "To terminate the progran use Ctrl+C.\n");
    signal(SIGINT, ctrl_c);
    create_ring_buffer();
    if(pthread_create(&prod_th, 0, producer, 0)) { //if failed producer thread, stop running
        perror("Create producer thread");
        run = 0;
    }
    
    if(pthread_create(&cons_th, 0, consumer, 0)) { //if failed consumer thread, stop running
        perror("Create consumer thread");
        run = 0;
    }
    pthread_join(prod_th, NULL);
    pthread_join(cons_th, NULL);
    RBufferDestroy(rb);
    TRACE(stderr, "END\n");
    return 0;
}

int create_ring_buffer()
{
    rb = RBufferCreate(1024, &comparator, 1);
    if(rb == NULL)
        return -1;
    return 0;
}

void* producer(void* ctx)
{
    assert(rb);
    TRACE(stderr, "Producer started\n");
    while(run) {
        usleep((rand() % 101) * 1000);           //0-100 ms (inclusive) in us
        int val = (int)rand() % 100 + 1;
        RBufferInsert(rb, &val);
    }
    TRACE(stderr, "Producer stopping\n");
    
    return 0;
}

void* consumer(void* ctx)
{
    assert(rb);
    TRACE(stderr, "Consumer started\n");
    int* val = malloc(sizeof(int));
    while (run) {
        usleep((rand() % 101) * 1000);           //0-100 ms (inclusive) in us
        RBufferGet(rb, (void*)&val);
        TRACE(stdout, "%d \n", *val);
    }
    TRACE(stderr, "Consumer stopping\n");
    
    return 0;
}
