#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

typedef struct callable {
    void *(*function)(void *, size_t, size_t *);
    void *arg;
    size_t argsz;
} callable_t;

typedef struct future {
    void *result;
    pthread_mutex_t mutex;
    pthread_cond_t to_finish;
} future_t;

typedef struct future_data {
    void *callable_arg;
    void *future_arg;
} future_data_t;

int async(thread_pool_t *pool, future_t *future, callable_t callable);

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *));

void *await(future_t *future);

#endif