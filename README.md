# ThreadPool
My task was to implement thread pool in C and to prepare 2 exaple programs to demonstrate how it works.
Thread pool allows to avoid creation and destruction of threads for each task.
Threads are created only at the beginning, which can result in better
performance and stability.
Thread Pool Description:

int thread_pool_init(thread_pool_t *pool, size_t pool_size) -
  initialization of thread pool with pool_size threads
  thread_pool_destroy - destruction of thread pool
  defer(pool, runnable) - orders thread pool to carry out task described by runnable argument
  Runnable is struct, that contains function,
  arguments and size of memory enable memory in this struct.


Future mechanism
Another part of this project was to implement future mechanism in C similar to that from JS.

int async(thread_pool_t *pool, future_t *future, callable_t callable) -
  initialization of memory indicated by future_t*
  and orders thread pool from argument to carry out task from callable argument.
  Function from callable returns pointer to result.

Now we can wait for the result - void *await(future_t *future) 
  or order this or other thread pool to call another function on the result -
  int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *))

Example programs:
1. Program macierz.c (matrix.c)
It takes two numbers k and n, which represent number of rows and columns, respectively.
Then program takes k*n lines. Each lines consists of two number v and t.
Number v in line number i (numbered from 0)
represent number in row floor(i/n) and column i mod n of matrix.
t is a number of milliseconds, which are neccessary to count value v.
Example:
2
3
1 2
1 5
12 4
23 9
3 11
7 2
This data represent matrix
|  1  1 12 |
| 23  3  7 |

Program matrix.c counts sum of rows and waits time indicated by value v in every single task of addition.

2. Program silnia.c counts asynchronously factorial of given as input number using map function.
