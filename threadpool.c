#include <stdbool.h>
#include "threadpool.h"
//Lista pul watkow
thread_pool_queue *queue = NULL;
//Zmienna trzymajaca informacje o tym czy pule
//zostaly juz zniszczone po odebraniu sygnalu
bool already_destroyed;

//Funkcja, ktora zwraca prace do wykonania
runnable_t *get_work (thread_pool_t *pool) {
    if (pool == NULL) {
        return NULL;
    }
    if (pool->first == NULL) {
        return NULL;
    }
    runnable_queue *work = pool->first;
    if (work->next == NULL) {
        pool->first = NULL;
        pool->last = NULL;
    } else {
        pool->first = work->next;
    }
    runnable_t *ret_work = work->runnable;
    free(work);
    return ret_work;
}

//Funkcja, ktora oblsuguje sygnal
//jest wykonywana przez glowny watek
void thread_catch_main() {
    while (queue != NULL) {
        thread_pool_queue *next_ptr = queue->next;
        thread_pool_destroy(queue->th_pool);
        free(queue);
        queue = next_ptr;
    }
    already_destroyed = true;
}

//Funckja, w ktorej dzilaja watki, wykonuja prace,
//oczekuja na nia i budza glowny watek do zniszczenia puli
static void *thread_exec(void* pool) {

    //wywoluje sigmask, zeby watki z puli ignorowaly SIGINT
    thread_pool_t *pool_ptr = pool;
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &block_mask, 0) == -1)
        printf("sigprocmask block");

    while (true) {

        if (pthread_mutex_lock(&(pool_ptr->mutex)) != 0) {
            printf("error1");
            return false;
        }
        pool_ptr->started = true;
        if (pool_ptr->finish == true) {
            break;
        }
        //Watki czekaja na prace
        while (pool_ptr->first == NULL) {
            if (pthread_cond_wait(&pool_ptr->wake, &pool_ptr->mutex) != 0) {
                printf("error2");
                return false;
            }
            if (pool_ptr->want_finish) {
                break;
            }
        }
        //Watek pobiera prace
        runnable_t *work = get_work(pool_ptr);
        (pool_ptr->working_threads)++;
        if (pthread_mutex_unlock(&(pool_ptr->mutex)) != 0) {
            printf("error3");
            return false;
        }

        //watek zaczyna wykonywanie pracy
        if (work != NULL) {
            work->function(work->arg, work->argsz);
            free(work);
        }


        if (pthread_mutex_lock(&(pool_ptr->mutex)) != 0) {
            printf("error4");
            return false;
        }

        //Budze glowny watek, zeby zniszczyl pule
        (pool_ptr->working_threads)--;
        if (pool_ptr->finish == false && pool_ptr->working_threads == 0 && pool_ptr->first == NULL) {
           if (pthread_cond_signal(&(pool_ptr->to_finish)) != 0) {
               printf("error5");
               return false;
           }

        }
       if (pthread_mutex_unlock(&(pool_ptr->mutex)) != 0) {
           printf("error6");
           return false;
       }
    }
    (pool_ptr->num_threads)--;
    //Budze glowny watek, czekaja
    if (pthread_cond_signal(&(pool_ptr->to_finish)) != 0) {
        printf("error7");
        return false;
    }
    if (pthread_mutex_unlock(&(pool_ptr->mutex)) != 0) {
        printf("error8");
        return false;
    }
    return (void*)true;
}

//Funkcja inicjalizujaca nowa pule
int thread_pool_init(thread_pool_t *pool, size_t pool_size) {
    if (pool == NULL) {
        pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
        if (pool == NULL) {
            return -1;
        }
    }
    //dodaje pule do globalnej listy pul
    thread_pool_queue *new_node = (thread_pool_queue *)malloc(sizeof(thread_pool_queue));
    if (new_node == NULL) {
        return -1;
    }
    new_node->th_pool = pool;
    new_node->next = queue;
    queue = new_node;
    //ustawiam funkcje obslugujaca SIGINT przez glowny watek
    struct sigaction action;
    sigset_t block_mask;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);

    action.sa_handler = thread_catch_main;
    action.sa_mask = block_mask;
    action.sa_flags = 0;

    if (sigaction (SIGINT, &action, 0) == -1) {
        printf("sigaction");
    }
    //ustawiam parametry struktur, inicjalizuje semafory i tworze nowe watki
    pool->threads = (pthread_t *)malloc(pool_size * sizeof(pthread_t));
    if (pool->threads == NULL) {
        printf("error");
        return -1;
    }
    pool->working_threads = 0;
    pool->num_threads = pool_size;
    pool->first = NULL;
    pool->last = NULL;
    pool->finish = false;
    pool->want_finish = false;
    pool->started = false;
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        printf("error13");
        return -1;
    }
    if (pthread_cond_init(&pool->wake, NULL) != 0) {
        printf("error14");
        return -1;
    }
    if (pthread_cond_init(&pool->to_finish, NULL) != 0) {
        printf("error15");
        return -1;
    }
    for (size_t i = 0; i < pool_size; ++i) {
        pthread_create(&pool->threads[i], NULL, thread_exec, pool);
        pthread_detach(pool->threads[i]);
    }
    return 0;
}

//Funkcja rejestrujaca nowa prace
int defer(thread_pool_t *pool, runnable_t runnable) {
    if (already_destroyed) {
        return 0;
    } else if (pool == NULL) {
        return -1;
    } else if (pool->want_finish) {
        return 0;
    }
    if (already_destroyed) {
        return -2;
    }
    runnable_t *new_work = (runnable_t*)malloc(sizeof(runnable_t));
    if (new_work == NULL) {
        return -1;
    }
    runnable_queue *new_node = (runnable_queue*)malloc(sizeof(runnable_queue));
    if (new_node == NULL) {
        return -1;
    }
    if (pthread_mutex_lock(&(pool->mutex)) != 0) {
        printf("error16");
        return -1;
    }
    new_work->arg = runnable.arg;
    new_work->function = runnable.function;
    new_work->argsz = runnable.argsz;
    new_node->runnable = new_work;
    new_node->next = NULL;
    if (pool->first == NULL) {
        pool->first = new_node;
        pool->last = new_node;
    } else {
        pool->last->next = new_node;
        pool->last = new_node;
    }
    pool->finish = false;
    //Budze watki do pracy
    if (pthread_cond_broadcast(&(pool->wake)) != 0) {
        printf("error17");
        return -1;
    }
    if (pthread_mutex_unlock(&(pool->mutex)) != 0) {
        printf("error18");
        return -1;
    }
    return 0;
}
//funkcja, w ktorej glowny watek czeka na koniec pracy watkow z puli
bool waiting(thread_pool_t *pool) {
    if (pthread_mutex_lock(&(pool->mutex)) != 0) {
        printf("error19");
        return false;
    }

    while ((!pool->finish && pool->working_threads != 0) || (pool->finish && pool->num_threads != 0) || !pool->started) {

        if (pthread_cond_wait(&(pool->to_finish), &(pool->mutex)) != 0) {
            printf("error20");
            return false;
        }
    }
    if (pthread_mutex_unlock(&(pool->mutex)) != 0) {
        printf("error21");
        return false;
    }
    return true;
}
//Funkcja, ktora czysci wezel, w ktorej byla usunieta pula
int delete_pool(thread_pool_t *pool) {
    thread_pool_queue *ptr = queue;
    thread_pool_queue *ptr_next;
    while (ptr != NULL) {
        ptr_next = ptr->next;
        if (ptr->th_pool == pool) {
            ptr->next = ptr_next;
            free(ptr);
            break;
        }
        ptr = ptr->next;
    }
    return 0;
}

//Funkcja niszczaca pule
void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL || already_destroyed) {
        return;
    }

    if (pthread_mutex_lock(&(pool->mutex)) != 0) {
        printf("error22");
        return;
    }
    pool->want_finish = true;
    //oczekuje na koniec pracy watkow
    while (pool->first != NULL) {
       if (pthread_cond_wait(&(pool->to_finish), &(pool->mutex)) != 0) {
           printf("error23");
           return;
       }
    }

    pool->finish = true;
    //budze watki, zeby sie zakonczyly
    if (pthread_cond_broadcast(&(pool->wake)) != 0) {
        printf("error26");
        return;
    }
    if (pthread_mutex_unlock(&(pool->mutex)) != 0) {
        printf("error27");
        return;
    }
    //czekam, az watki sie zakoncza
    if (waiting(pool) != true) {
        printf("error28");
        return;
    }

    runnable_queue *ptr1 = pool->first;
    runnable_queue *ptr2;
    while (ptr1 != NULL) {
        ptr2 = ptr1->next;
        free(ptr1->runnable);
        free(ptr1);
        ptr1 = ptr2;
    }
    int err = delete_pool(pool);
    if (err != 0) {
        printf("error");
    }
    //niszcze semafory
    if (pthread_mutex_destroy(&(pool->mutex)) != 0) {
        printf("error29");
        return;
    }
    if (pthread_cond_destroy(&(pool->wake)) != 0) {
        printf("error30");
        return;
    }
    if (pthread_cond_destroy(&(pool->to_finish)) != 0) {
        printf("error31");
        return;
    }
    free(pool->threads);
}
