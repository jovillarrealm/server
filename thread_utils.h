#ifndef THREAD_UTILS
#define THREAD_UTILS
#include <sys/types.h>
#include <pthread.h>
// Evil
#define MAX_EVENTS 128
#define MAX_THREADS 1
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

void *worker_thread(void *arg);

#endif