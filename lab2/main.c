#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <pthread.h>
#include <signal.h>

#include "list.h"
#include "board.h"

#define THREAD_NUM 3

void *(*thread_functions[THREAD_NUM])(void*);
pthread_t threads[THREAD_NUM];

pthread_mutex_t m_board;
pthread_cond_t u_human, u_computer;

struct list_t *q_worker_ready;
struct board_t board;

int RUNNING;
enum player_t CURRENT_PLAYER;

void Exit(int sig) {
    int myid;

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (myid == 0) {
        ListDestruct(&q_worker_ready);
        BoardEnd();
    }

    /* somebody won */
    if (RUNNING == 0) {
        if (CURRENT_PLAYER == HUMAN) {
            printf("Computer won!\n");
        } else if (CURRENT_PLAYER == COMPUTER) {
            printf("Human won!\n");
        } else {
            printf("Draw!\n");
        }
    }

    RUNNING = 0;

    if (MPI_Finalize() != MPI_SUCCESS) {
        errx(1, "MPI_Finalize failed!");
    }

    exit(0);
}

void *worker(void *arg) {
    /*
     * recv type = RACUNAJ ili KRAJ
     * recv board
     * recv player_turn
     * recv depth
     *
     * evaluate
     *
     * send result
     */

    return NULL;
}

void *master(void *arg) {
    /*
     * foreach free_column as c:
     *  find free worker
     *  make a move as computer on c
     *  send him board, computer, depth
     *  undo move as computer on c
     *
     *  recv current result
     *  update best result
     *
     * make a move on best result as computer
     * let player play
     */

    int best_col, best_result;
    int worker_id;

    while (RUNNING) {
        pthread_mutex_lock(&m_board);

        while (CURRENT_PLAYER != COMPUTER) {
            pthread_cond_wait(&u_computer, &m_board);
        }

        if (RUNNING == 0) {
            pthread_mutex_unlock(&m_board);
            break;
        }

        /* start of dummy moves */
        for (best_col = 0; best_col < BOARD_COLS; ++best_col) {
            if (BoardIsValidMove(&board, best_col)) {
                break;
            }
        }
        /* end of dummy moves */

        BoardMove(&board, best_col, COMPUTER);
        BoardPrint(&board);

        if (BoardIsGameOver(&board, best_col)) {
            RUNNING = 0;
        }

        CURRENT_PLAYER = HUMAN;
        pthread_cond_signal(&u_human);
        pthread_mutex_unlock(&m_board);
    }

    /* fire all workers */
    while (ListEmpty(&q_worker_ready) == 0) {
        worker_id = ListHead(&q_worker_ready);

        /*
         * send message type that determines the end
         */

        ListRemove(&q_worker_ready, worker_id);
    }

    return NULL;
}

void *user_input(void *arg) {
    /*
     * draw board
     * wait for user input
     * update drawing
     *
     * let computer play
     */
    int col;

    while (RUNNING) {
        pthread_mutex_lock(&m_board);

        while (CURRENT_PLAYER != HUMAN) {
            pthread_cond_wait(&u_human, &m_board);
        }

        if (RUNNING == 0) {
            pthread_mutex_unlock(&m_board);
            break;
        }

        do {
            col = BoardInput() - '1';
        } while(BoardIsValidMove(&board, col) == 0);

        BoardMove(&board, col, HUMAN);
        BoardPrint(&board);

        if (BoardIsGameOver(&board, col)) {
            RUNNING = 0;
        }

        CURRENT_PLAYER = COMPUTER;
        pthread_cond_signal(&u_computer);
        pthread_mutex_unlock(&m_board);
    }

    return NULL;
}

int main(int argc, char **argv) {
    int myid, n_workers, status, n_thread, i;

    if (MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &status) != MPI_SUCCESS) {
        errx(1, "MPI_Init_thread failed!");
    }

    if (status != MPI_THREAD_MULTIPLE) {
        errx(1, "MPI_Init_thread failed to provide thread support!");
    }

    thread_functions[0] = worker;
    thread_functions[1] = master;
    thread_functions[2] = user_input;

    pthread_mutex_init(&m_board, NULL);
    pthread_cond_init(&u_human, NULL);
    pthread_cond_init(&u_computer, NULL);

    sigset(SIGINT, Exit);

    MPI_Comm_size(MPI_COMM_WORLD, &n_workers);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    /* init processes */
    if (myid == 0) {
        ListConstruct(&q_worker_ready);

        for (i = 0; i < n_workers; ++i) {
            ListInsert(&q_worker_ready, i);
        }

        BoardInit(&board);
        BoardPrint(&board);

        n_thread = 3;
    } else {
        n_thread = 1;
    }

    /* start the game */
    RUNNING = 1;
    CURRENT_PLAYER = HUMAN;

    for (i = 0; i < n_thread; ++i) {
        if (pthread_create(&threads[i], NULL, thread_functions[i], NULL) != 0) {
            errx(1, "pthread_create(): failed to create a new thread!");
        }
    }

    /* end the game */
    for (i = 0; i < n_thread; ++i) {
        pthread_join(threads[i], NULL);
    }

    Exit(0);
}
