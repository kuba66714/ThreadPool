#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
typedef struct runnable {
    void (*function)(void *, size_t);
    void *arg;
    size_t argsz;
} runnable_t;
//lista prac
typedef struct runnable_queue {
    runnable_t *runnable;
    struct runnable_queue *next;
}runnable_queue;

typedef struct thread_pool {
    //mutex na zmienne
    pthread_mutex_t mutex;
    //zmienna warunkowa, na ktorej czekaja
    //watki z puli na prace
    pthread_cond_t wake;
    //zmienna warunkowa, na ktorej czeka
    //glowny watek na zakonczenie puli
    pthread_cond_t to_finish;
    //wskaznik na watki
    pthread_t *threads;
    //liczba dzialajacych watkow
    size_t num_threads;
    //liczba pracujacych watkow
    size_t working_threads;
    //wskaznik na pierwsza prace
    runnable_queue *first;
    //wskaznik na ostatnia prace
    runnable_queue *last;
    //zmienna, ktora trzyma informacje
    //czy pula ma byc zniszczona
    bool finish;
    //zmienna, ktora trzyma informacje
    //czy praca sie rozpoczela
    bool started;
    //zmienna, ktora trzyma informacje
    //czy mozna dodawac kolejne prace
    bool want_finish;
} thread_pool_t;

//lista pul watkow
typedef struct thread_pool_queue {
    thread_pool_t *th_pool;
    struct thread_pool_queue *next;

} thread_pool_queue;


int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

#endif