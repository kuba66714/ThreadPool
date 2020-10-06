#include "future.h"

int count;
pthread_mutex_t mutex;

//funkcja mnozaca kolejne czesciowe iloczyny
static void *multiply(void *arg, size_t argsz __attribute__((unused)),
                      size_t *retsz __attribute__((unused))) {
    int m;
    pthread_mutex_lock(&mutex);
    m = count;
    count--;
    pthread_mutex_unlock(&mutex);
    int *in = arg;
    int prev = in[0];
    free(arg);
    int *ret = malloc(sizeof(int));
    if (m == 0) {
        m = 1;
    }
    *ret = prev * m;
    return ret;
}

int main() {
    int n;
    scanf("%d", &n);
    thread_pool_t pool;
    future_t future;
    count = n;
    //tworze pule 3 watkow
    thread_pool_init(&pool, 3);
    pthread_mutex_init(&mutex, NULL);
    int *arg = malloc(sizeof(int*));
    int prev = 1;
    arg[0] = prev;
    void *buf = arg;
    //wykonuje pierwsze mnozenie i tworze pierwszy future
    async(&pool, &future, (callable_t){.function = multiply,
            .arg = buf, .argsz = sizeof(int)
    });
    future_t *f1 = &future;
    future_t *ost = &future;
    //zlecam kolejne przemnozenia
    for (int i = 2; i < n; ++i) {
        future_t f2;
        ost = &f2;
        map(&pool, &f2, f1, multiply);
        f1 = &f2;
    }
    int *res = await(ost);

    printf("%d\n", *res);
    free(res);
    thread_pool_destroy(&pool);
    pthread_mutex_destroy(&mutex);


    return 0;
}