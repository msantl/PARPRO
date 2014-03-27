#include "list.h"

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <err.h>
#include <time.h>

#define REQUEST         0
#define RESPONSE        1
#define THREAD_NUM      4
#define MAX_BUFF        256
#define MIN_SLEEP       1000
#define MAX_SLEEP       500000

void *(*thread_functions[THREAD_NUM])(void*);
pthread_t threads[THREAD_NUM];
pthread_mutex_t m_request;

struct list_t *request_list;

pthread_cond_t u_forks, u_thinking;
int left_fork, right_fork, thinking;

int n_filozofa;
int left_id, right_id;

int RUNNING;

void randomSleep(void) { usleep(MIN_SLEEP + (rand() % (MAX_SLEEP - MIN_SLEEP))); }

void *filozof(void *arg) {
    int myid;

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    srand((myid + 1) * (unsigned) time(NULL));

    while (RUNNING) {
        printf("Filozof %d misli\n", myid);
        randomSleep();

        pthread_mutex_lock(&m_request);

        printf("Filozof %d nabavlja vilice (%d, %d)\n", myid, left_fork, right_fork);
        thinking = 0;

        if (left_fork == 0) {
#ifdef DEBUG
            printf("Filozof %d salje zahtjev %d!\n", myid, left_id);
#endif
            MPI_Send(&myid, 1, MPI_INT, left_id, REQUEST, MPI_COMM_WORLD);
        }

        if (right_fork == 0) {
#ifdef DEBUG
            printf("Filozof %d salje zahtjev %d!\n", myid, right_id);
#endif
            MPI_Send(&myid, 1, MPI_INT, right_id, REQUEST, MPI_COMM_WORLD);
        }

        while (left_fork == 0 || right_fork == 0) {
            pthread_cond_wait(&u_forks, &m_request);
        }

        left_fork = right_fork = 0;
        pthread_mutex_unlock(&m_request);

        printf("Filozof %d jede\n", myid);
        randomSleep();

        pthread_mutex_lock(&m_request);

        left_fork = right_fork = 1;
        thinking = 1;
        pthread_cond_signal(&u_thinking);

        pthread_mutex_unlock(&m_request);
    }

    return NULL;
}

void *worker(void *arg) {
    int myid;

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    while (RUNNING) {
        pthread_mutex_lock(&m_request);

        while (thinking == 0) {
            pthread_cond_wait(&u_thinking, &m_request);
        }

        /* daj vilicu susjedu ako je treba */
        if (left_fork && ListFind(&request_list, left_id)) {
            ListRemove(&request_list, left_id);

#ifdef DEBUG
            printf("Filozof %d salje vilicu %d\n", myid, left_id);
#endif
            MPI_Send(&myid, 1, MPI_INT, left_id, RESPONSE, MPI_COMM_WORLD);
            left_fork = 0;
        }

        /* daj vilicu susjedu ako je treba */
        if (right_fork && ListFind(&request_list, right_id)) {
            ListRemove(&request_list, right_id);

#ifdef DEBUG
            printf("Filozof %d salje vilicu %d\n", myid, right_id);
#endif
            MPI_Send(&myid, 1, MPI_INT, right_id, RESPONSE, MPI_COMM_WORLD);
            right_fork = 0;
        }

        pthread_mutex_unlock(&m_request);
    }

    return NULL;
}

void *request_listener(void *arg) {
    int id, myid;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    while (RUNNING) {
        /* primi poruku */
        MPI_Recv(&id, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);
#ifdef DEBUG
        printf("Filozof %d dobio zahtjev od %d\n", myid, id);
#endif

        pthread_mutex_lock(&m_request);

        ListInsert(&request_list, id);

        pthread_mutex_unlock(&m_request);
    }

    return NULL;
}

void *response_listener(void *arg) {
    int id, myid;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    while (RUNNING) {
        /* primi poruku */
        MPI_Recv(&id, 1, MPI_INT, MPI_ANY_SOURCE, RESPONSE, MPI_COMM_WORLD, &status);
#ifdef DEBUG
        printf("Filozof %d je dobio vilicu od %d\n", myid, id);
#endif

        pthread_mutex_lock(&m_request);

        if (id == left_id) {
            left_fork = 1;
        } else if (id == right_id) {
            right_fork = 1;
        } else {
            warnx("Primljena poruka nije od susjeda!");
        }

        if (left_fork && right_fork) {
            pthread_cond_signal(&u_forks);
        }
        pthread_mutex_unlock(&m_request);
    }

    return NULL;
}

void Exit(int sig) {
    RUNNING = 0;

    pthread_mutex_lock(&m_request);
    ListDestruct(&request_list);
    pthread_mutex_unlock(&m_request);

    MPI_Finalize();

    warnx("Now exiting!");
    exit(0);
}

int main(int argc, char **argv) {
    int i, myid;

    pthread_mutex_init(&m_request, NULL);
    pthread_cond_init(&u_forks, NULL);
    pthread_cond_init(&u_thinking, NULL);

    ListConstruct(&request_list);

    thread_functions[0] = filozof;
    thread_functions[1] = request_listener;
    thread_functions[2] = response_listener;
    thread_functions[3] = worker;

    RUNNING = 1;

    sigset(SIGINT, Exit);

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &n_filozofa);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    left_id = (myid - 1 + n_filozofa) % n_filozofa;
    right_id = (myid + 1) % n_filozofa;

    /* pocetno stanje vilica */
    if (myid == 0) {
        left_fork = right_fork = 1;
    } else if (myid == n_filozofa - 1) {
        left_fork = right_fork = 0;
    } else {
        left_fork = 0;
        right_fork = 1;
    }

    /* na pocetku svi misle */
    thinking = 1;

    /* neka gozba pocne */
    for (i = 0; i < THREAD_NUM; ++i) {
        if (pthread_create(&threads[i], NULL, thread_functions[i], NULL) != 0) {
            errx(1, "pthread_create(): failed to create a new thread!");
        }
    }

    for (i = 0; i < THREAD_NUM; ++i) {
        pthread_join(threads[i], NULL);
    }

    Exit(0);
}
