/***
 *
 * Multi-threaded limited circular buffer,
 * with one producer and one consumer.
 * 
 **/


/*
 *  ring buffer handle used to keep a pointer to the ring buffer instance
 *  */
typedef void* rbhandle_t;

/**
 *   Compare two ring buffer items
 *     
 *       Returns:
          
 *       0 if entries are equal
 *       1 if *e1 > *e2
 *      -1 if *e1 < *e2
 */
typedef int (*ecomp_t) (void* e1, void* e2);

/**
 *   Create a new ring buffer instance
 */
rbhandle_t
RBufferCreate(int buf_size, ecomp_t comp, int entry_size);

/**
 *  Insert new entry to the ring buffer
 */
int
RBufferInsert(rbhandle_t h, void* e);

/**
 *   Get an entry from the ring buffer
 *     Blocking call. It waits for new entry if the buffer is empty
 */

int
RBufferGet(rbhandle_t h, void** e);

/**
 *   Get an entry from the ring buffer
 *    Non-blocking call. Returns immediatelly if buffer is empty
 */

int
RBufferTryGet(rbhandle_t h, void** e);

/**
 *    Destroy the ring buffer
 *
 */
void
RBufferDestroy(rbhandle_t h);
