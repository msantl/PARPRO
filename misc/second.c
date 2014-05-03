#include <mpi.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct board_t {
    int a[10][10];
};

void initBoard(struct board_t *board) {
    int i, j;
    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 10; ++j) {
            board->a[i][j] = i * j - j - i;
        }
    }
    return;
}

void printBoard(struct board_t *board) {
    int i, j;
    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 10; ++j) {
            printf("%4d", board->a[i][j]);
        }
        printf("\n");
    }
    return;
}

enum player_t {FIRST = 'a', SECOND = 'z'};

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int myid, n_workers, i;
    struct board_t board;
    enum player_t player;

    MPI_Comm_size(MPI_COMM_WORLD, &n_workers);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    printf("%ld %ld\n", sizeof(board), sizeof(player));

    switch (myid) {
        case 0:
            /* master */
            initBoard(&board);
            player = SECOND;

            printf("-------------\n");
            printf("---MASTER--%c-\n", player);
            printf("-------------\n");
            printBoard(&board);
            printf("-------------\n");

            for (i = 1; i < n_workers; ++i) {
                MPI_Send((int *)&board, sizeof(board), MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send((int *)&player, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }

            break;
        default:
            /* worker */
            MPI_Recv((int *)&board, sizeof(board), MPI_INT, MPI_ANY_SOURCE, 0,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            MPI_Recv((int *)&player, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);

            printf("-------------\n");
            printf("---WORKER--%c-\n", player);
            printf("-------------\n");
            printBoard(&board);
            printf("-------------\n");

            break;
    }

    MPI_Finalize();
    return 0;
}
