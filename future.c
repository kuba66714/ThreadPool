//
// Created by kuba on 27.12.19.
//

#include "future.h"

typedef void *(*function_t)(void *);

//Funkcja, ktora wywoluje zlecona funkcje
//i zapisuje wynik w future
void runnable_func(void *args, size_t argsz) {
    if (argsz == 0) {
        return;
    }
    //tworze strukture, w ktorej przekazuje argumenty
    future_data_t *fdt = args;
    callable_t *callable_arg = fdt->callable_arg;
    future_t *future = fdt->future_arg;
    free(fdt);
    size_t future_size;
    //wywoluje zlecona funckcje
    void *result = callable_arg->function(callable_arg->arg, callable_arg->argsz, &future_size);

    free(callable_arg);
    if (pthread_mutex_lock(&future->mutex) != 0) {
        printf("error");
        return;
    }
    bool wake_up = false;
    if (future->result == NULL) {
        wake_up = true;
    }
    future->result = result;
    if (wake_up) {
        if (pthread_cond_signal(&future->to_finish) != 0) {
            printf("error");
            return;
        }
    }
    if (pthread_mutex_unlock(&future->mutex) != 0) {
        printf("error");
        return;
    }
}

int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    if (pool == NULL) {
        return 1;
    }
    if (pthread_mutex_init(&future->mutex, NULL) != 0) {
        printf("error");
        return -1;
    }
    //inicjuje semafor
    pthread_cond_init(&future->to_finish, NULL);
    future->result = NULL;
    future_data_t *fdt = (future_data_t *)malloc(sizeof(future_data_t));
    if (fdt == NULL) {
        return 1;
    }
    //inicjuje nowa strukture callable
    callable_t *new_callable = (callable_t*)malloc(sizeof(callable_t));
    if (new_callable == NULL) {
        printf("error");
        return -1;
    }
    //ustwiam parametry callable
    new_callable->arg = callable.arg;
    new_callable->argsz = callable.argsz;
    new_callable->function = callable.function;
    fdt->callable_arg = new_callable;
    fdt->future_arg = future;
    //zlecam nowa prace
    int err = defer(pool, (runnable_t){ .function = runnable_func,
            .arg = fdt, .argsz = sizeof(fdt)
    });
    if (err != 0) {
        printf("error");
        return -1;
    }
    return 0;
}

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *)) {
    if (pool == NULL || future == NULL || from == NULL) {
        return 1;
    }
    //czekam, az w future from bedzie gotowy wynik
    void *res = await(from);
    if (res == NULL) {
        printf("error");
        return -1;
    }

    callable_t callable;
    callable.function = function;
    callable.arg = res;
    callable.argsz = sizeof(res);
    //zlecam nowa prace i zapisanie rezultatu w future
   int err = async(pool, future, callable);
   if (err != 0) {
       printf("error");
       return -1;
   }

    return 0;
}

void *await(future_t *future) {
    if (pthread_mutex_lock(&future->mutex) != 0) {
        printf("error");
        return NULL;
    }
    //oczekuje na zakonczenie pracy
    while (future->result == NULL) {
        if (pthread_cond_wait(&future->to_finish, &future->mutex) != 0) {
            printf("error");
            return NULL;
        }
    }
    void *ret = future->result;
    if (pthread_mutex_unlock(&future->mutex) != 0) {
        printf("error");
        return NULL;
    }
    return ret;
}