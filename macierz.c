
#include "threadpool.h"

//struktura trzymajaca dane do pracy
typedef struct data_s {
    int val;
    int time;
    pthread_mutex_t *semaphores;
    int *sums;
    int actK;
    struct data_s *next;
}data;

//tworze nowa prace
data *create_work (int actK, int val, int time, int *sums, pthread_mutex_t *sems, data *prev) {
    data *dt = (data *)malloc(sizeof(data));
    dt->actK = actK;
    dt->val= val;
    dt->time = time;
    dt->sums = sums;
    dt->semaphores = sems;
    dt->next = prev;
    return dt;
}
//funkcja sumujaca wiersze macierzy
static void count_matrix (void *args, size_t argsz) {
    if (argsz == 0) {
        return;
    }
    data *dt = args;
    usleep(dt->time);
    pthread_mutex_lock(&dt->semaphores[dt->actK]);
    dt->sums[dt->actK] += dt->val;

    pthread_mutex_unlock(&dt->semaphores[dt->actK]);
}

int main () {
    int k, n;
    scanf("%d", &k);
    scanf("%d", &n);
    int *sums = (int *)malloc(k * sizeof(int));
    //tworze semafory, na kazdy wiersz macierzy
    pthread_mutex_t *semaphores = (pthread_mutex_t *)malloc(k * sizeof(pthread_mutex_t));
    for (int i = 0; i < k; ++i) {
        pthread_mutex_init(&semaphores[i], NULL);
    }
    //suma dla kazdego wiersza macierzy
    for (int i = 0; i < k; ++i) {
        sums[i] = 0;
    }
    //tworze nowa pule, skladajaca sie z 4 watkow
    thread_pool_t pool;
    thread_pool_init(&pool, 4);
    data *prev = NULL;
    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < n; ++j) {
            int v, t;
            scanf("%d", &v);
            getchar();
            scanf("%d", &t);
            //tworze nowa prace
            data *dt = create_work(i, v, t, sums, semaphores, prev);
            prev = dt;
            //zlecam wykonanie tej pracy
            int err = defer(&pool, (runnable_t) {.function = count_matrix,
                    .arg = dt, .argsz = sizeof(dt)});
            if (err != 0) {
                printf("error");
                break;
            }
        }
    }
    //niszcze pule watkow
    thread_pool_destroy(&pool);
    for (int i = 0; i < k; ++i) {
        printf("%d\n", sums[i]);
    }

    free(sums);
    //niszcze semafory
    for (int i = 0; i < k; ++i) {
        pthread_mutex_destroy(&semaphores[i]);
    }
    free(semaphores);
    data *ptr;
    //niszcze zaalokowane struktury pracy
    while (prev != NULL) {
        ptr = prev->next;
        free(prev);
        prev = ptr;
    }
    return 0;
}