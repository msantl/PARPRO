#ifndef __BOARD_H
#define __BOARD_H

#define BOARD_ROWS 6
#define BOARD_COLS 7

#define startX      0
#define startY      0

enum player_t {EMPTY = '.', COMPUTER = 'x', HUMAN = 'o'};

struct board_t {
    char a[BOARD_ROWS][BOARD_COLS];
};

void BoardInit(struct board_t *);

void BoardEnd(void);

void BoardMove(struct board_t *, int, enum player_t);

int BoardIsValidMove(struct board_t *, int);

int BoardIsGameOver(struct board_t *, int);

void BoardPrint(struct board_t *);

char BoardInput(void);

#endif
