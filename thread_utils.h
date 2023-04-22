#ifndef THREAD_UTILS
#define THREAD_UTILS
#include <sys/types.h>
// Evil
#define MAX_EVENTS 128
#define MAX_THREADS 2
#define QUEUE_SIZE 256


typedef struct ServerState
{
    int epoll_fd;
    int listen_fd;
    int queue[QUEUE_SIZE];
    int queue_front;
    int queue_rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FILE *log;
    char *doc_root;
} ServerState;

// EVIL bitchcraft Do nOt eaT
void *worker_thread(void *arg);

#endif