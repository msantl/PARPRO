#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "list.h"
#include "board.h"

#define WORKER_START    1
#define WORKER_QUIT     2

#define REQUEST         50

#define THREAD_NUM      3
#define MAX_DEPTH       6

int g_max_depth;

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

    /* somebody won */
    if (RUNNING == 0) {
        if (CURRENT_PLAYER == HUMAN) {
            BoardOutput("Computer won!\n");
        } else if (CURRENT_PLAYER == COMPUTER) {
            BoardOutput("Human won!\n");
        } else {
            BoardOutput("Draw!\n");
        }
    }

    if (myid == 0) {
        ListDestruct(&q_worker_ready);
        BoardEnd();
    }

    RUNNING = 0;

    if (MPI_Finalize() != MPI_SUCCESS) {
        errx(1, "MPI_Finalize failed!");
    }

    exit(0);
}

double EvaluateBoard(struct board_t *current, enum player_t last, int last_col, int depth) {
    double result, total;
    enum player_t now;
    int all_win = 1, all_lose = 1;
    int moves, col;

    if (BoardIsGameOver(current, last_col)) {
        if (last == COMPUTER) {
            return 1;
        } else {
            return -1;
        }
    }

    if (depth == 0) {
        return 0;
    }

    if (last == COMPUTER) {
        now = HUMAN;
    } else {
        now = COMPUTER;
    }

    total = 0;
    moves = 0;

    for (col = 0; col < BOARD_COLS; ++col) {
        if (BoardIsValidMove(current, col)) {
            moves ++;

            BoardMove(current, col, now);
            result = EvaluateBoard(current, now, col, depth - 1);
            BoardUndoMove(current, col);

            if (result > -1) all_lose = 0;
            if (result <  1) all_win  = 0;

            if (result ==  1 && now == COMPUTER) return  1;
            if (result == -1 && now == HUMAN)    return -1;

            total += result;
        }
    }

    if (all_win)  return  1;
    if (all_lose) return -1;

    return total / moves;
}

void *worker(void *arg) {
    struct board_t w_board;
    enum player_t player;
    int depth, col, type;

    double result;

    while (1) {
        MPI_Recv(&type, 1, MPI_INT, MPI_ANY_SOURCE,
            REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (type == WORKER_QUIT) {
            break;
        } else if (type == WORKER_START) {
            MPI_Recv((char *)&w_board, sizeof(board), MPI_CHAR, MPI_ANY_SOURCE,
                REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv((char *)&player, sizeof(player), MPI_CHAR, MPI_ANY_SOURCE,
                REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&col, 1, MPI_INT, MPI_ANY_SOURCE,
                REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&depth, 1, MPI_INT, MPI_ANY_SOURCE,
                REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            BoardMove(&w_board, col, player);
            result = EvaluateBoard(&w_board, player, col, depth);

            MPI_Send(&result, 1, MPI_DOUBLE, 0, col, MPI_COMM_WORLD);
        }
    }

    return NULL;
}

void *master(void *arg) {
    int jobs, best_col, col, worker_id, type, depth;
    int n_workers;
    double best_result, result;
    double t_start, t_end;
    enum player_t player = COMPUTER;
    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &n_workers);

    fprintf(stderr, "DEPTH = %d\n", g_max_depth);
    fprintf(stderr, "N_PROC = %d\n", n_workers);

    while (RUNNING) {
        pthread_mutex_lock(&m_board);

        while (CURRENT_PLAYER != COMPUTER) {
            pthread_cond_wait(&u_computer, &m_board);
        }

        if (BoardIsDraw(&board)) {
            RUNNING = 0;
            CURRENT_PLAYER = EMPTY;
        }

        if (RUNNING == 0) {
            pthread_mutex_unlock(&m_board);
            break;
        }

        /* start workers */
        best_col = -1;
        jobs = 0;
        depth = g_max_depth;

        /* measure start time here */
        t_start = MPI_Wtime();

        do {

            for (col = 0; col < BOARD_COLS; ++col) {
                if (BoardIsValidMove(&board, col)) {

                    /* find an available worker */
                    if (ListEmpty(&q_worker_ready) == 1) {
                        MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
                                MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        if (best_col == -1 || result > best_result) {
                            best_result = result;
                            best_col = status.MPI_TAG;
                        }

                        jobs --;

                        ListInsert(&q_worker_ready, status.MPI_SOURCE);
                    }

                    worker_id = ListHead(&q_worker_ready);
                    type = WORKER_START;

                    MPI_Send(&type, 1, MPI_INT, worker_id, REQUEST, MPI_COMM_WORLD);
                    /* board */
                    MPI_Send((char *)&board, sizeof(board), MPI_CHAR, worker_id, REQUEST, MPI_COMM_WORLD);
                    /* player */
                    MPI_Send((char *)&player, sizeof(player), MPI_CHAR, worker_id, REQUEST, MPI_COMM_WORLD);
                    /* column */
                    MPI_Send(&col, 1, MPI_INT, worker_id, REQUEST, MPI_COMM_WORLD);
                    /* depth */
                    MPI_Send(&depth, 1, MPI_INT, worker_id, REQUEST, MPI_COMM_WORLD);

                    jobs ++;

                    ListRemove(&q_worker_ready, worker_id);
                }
            }

            /* collect all results */
            while (jobs > 0) {
                MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
                        MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if (best_col == -1 || result > best_result) {
                    best_result = result;
                    best_col = status.MPI_TAG;
                }

                jobs --;

                ListInsert(&q_worker_ready, status.MPI_SOURCE);
            }

            depth >>= 1;

        } while (best_col == -1 && depth > 0);
        /* end */

        /* measure end time here */
        t_end = MPI_Wtime();

        /* output time to stderr */
        fprintf(stderr, "%.6lf\n", t_end - t_start);

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
        type = WORKER_QUIT;

        /* send message type that determines the end */
        MPI_Send(&type, 1, MPI_INT, worker_id, REQUEST, MPI_COMM_WORLD);

        ListRemove(&q_worker_ready, worker_id);
    }

    return NULL;
}

void *user_input(void *arg) {
    int col;

    while (RUNNING) {
        pthread_mutex_lock(&m_board);

        while (CURRENT_PLAYER != HUMAN) {
            pthread_cond_wait(&u_human, &m_board);
        }

        if (BoardIsDraw(&board)) {
            RUNNING = 0;
            CURRENT_PLAYER = EMPTY;
        }

        if (RUNNING == 0) {
            pthread_mutex_unlock(&m_board);
            break;
        }

        do { col = BoardInput() - '1'; } while(BoardIsValidMove(&board, col) == 0);

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

    if (argc == 2) {
        sscanf(argv[1], "%d", &g_max_depth);
    } else {
        g_max_depth = MAX_DEPTH;
    }

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
