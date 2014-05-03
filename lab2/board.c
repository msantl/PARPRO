#include "board.h"

#include <ncurses.h>
#include <unistd.h>

static void color_on(char c) {
    if (c == HUMAN) {
        attron(COLOR_PAIR(1));
    } else if (c == COMPUTER) {
        attron(COLOR_PAIR(2));
    }
}

static void color_off(char c) {
    if (c == HUMAN) {
        attroff(COLOR_PAIR(1));
    } else if (c == COMPUTER) {
        attroff(COLOR_PAIR(2));
    }
}

void BoardInit(struct board_t *board) {
    int i, j;

    for (i = 0; i < BOARD_ROWS; ++i) {
        for (j = 0; j < BOARD_COLS; ++j) {
            board->a[i][j] = EMPTY;
        }
    }

    /* start curses mode */
    initscr();
    start_color();

    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    clear();
    noecho();
    raw();
    curs_set(0);

    return;
}

void BoardEnd(void) {
    /* end curses mode */
    endwin();

    return;
}

void BoardMove(struct board_t *board, int col, enum player_t player) {
    int i;

    for (i = BOARD_ROWS - 1; i >= 0 && board->a[i][col] == EMPTY; --i);

    board->a[i + 1][col] = player;

    return;
}

void BoardUndoMove(struct board_t *board, int col) {
    int i;

    for (i = BOARD_ROWS - 1; i >= 0 && board->a[i][col] == EMPTY; --i);

    board->a[i][col] = EMPTY;

    return;
}

int BoardIsValidMove(struct board_t *board, int col) {
    int i;

    if (col < 0 || col >= BOARD_COLS) {
        return 0;
    }

    for (i = BOARD_ROWS - 1; i >= 0; --i) {
        if (board->a[i][col] == EMPTY) {
            return 1;
        }
    }

    return 0;
}

int BoardIsGameOver(struct board_t *board, int col) {
    int row;
    int x1, x2, y1, y2;

    for (row = BOARD_ROWS - 1; row >= 0 && board->a[row][col] == EMPTY; --row);

    /* rows */
    y1 = row;   y2 = row;
    while (y1 < BOARD_ROWS && board->a[y1][col] == board->a[row][col]) y1 ++;
    while (y2 >= 0 && board->a[y2][col] == board->a[row][col]) y2 --;

    if (y1 - y2 - 1 >= 4) return 1;

    /* cols */
    x1 = col;   x2 = col;
    while (x1 < BOARD_COLS && board->a[row][x1] == board->a[row][col]) x1 ++;
    while (x2 >= 0 && board->a[row][x2] == board->a[row][col]) x2 --;

    if (x1 - x2 - 1 >= 4) return 1;

    /* main diagonal */
    x1 = col;   x2 = col;
    y1 = row;   y2 = row;

    while (x1 < BOARD_COLS && y1 < BOARD_ROWS && board->a[y1][x1] == board->a[row][col]) x1 ++, y1 ++;
    while (x2 >= 0 && y2 >= 0 && board->a[y2][x2] == board->a[row][col]) x2 --, y2 --;

    if (y1 - y2 - 1 >= 4) return 1;

    /* other diagonal*/
    x1 = col;   x2 = col;
    y1 = row;   y2 = row;

    while (x1 >= 0 && y1 < BOARD_ROWS && board->a[y1][x1] == board->a[row][col]) x1 --, y1 ++;
    while (x2 < BOARD_COLS && y2 >= 0 && board->a[y2][x2] == board->a[row][col]) x2 ++, y2 --;

    if (x2 - x1 - 1 >= 4) return 1;

    return 0;
}

int BoardIsDraw(struct board_t *board) {
    int i;
    for (i = 0; i < BOARD_COLS; ++i) {
        if (board->a[BOARD_ROWS - 1][i] == EMPTY) return 0;
    }
    return 1;
}

void BoardPrint(struct board_t *board) {
    clear();
    int i, j;

    for (i = BOARD_ROWS - 1; i >= 0; --i) {
        for (j = 0; j < BOARD_COLS; ++j) {
            move(startY + BOARD_ROWS - i - 1, startX + 2*j);

            color_on(board->a[i][j]);
            addch(board->a[i][j]);
            color_off(board->a[i][j]);
        }
    }

    for (j = 0; j < BOARD_COLS; ++j) {
        move(startY + BOARD_ROWS, startX + 2*j);
        addch(j + '1');
    }

    move(startY + BOARD_ROWS + 1, startX);

    refresh();
    return;
}

char BoardInput(void) {
    int ch = getch();
    return ch;
}

void BoardOutput(char *c) {
    move(startY + BOARD_ROWS + 1, startX);
    printw(c);
    refresh();
    usleep(2000000);
}
