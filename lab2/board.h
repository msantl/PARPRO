#ifndef __BOARD_H
#define __BOARD_H

#define BOARD_ROWS  6
#define BOARD_COLS  7

#define startX      10
#define startY      5

enum player_t {EMPTY = '.', COMPUTER = 'x', HUMAN = 'o'};

struct board_t {
    char a[BOARD_ROWS][BOARD_COLS];
};

void BoardInit(struct board_t *);

void BoardEnd(void);

void BoardMove(struct board_t *, int, enum player_t);

void BoardUndoMove(struct board_t *, int);

int BoardIsValidMove(struct board_t *, int);

int BoardIsGameOver(struct board_t *, int);

int BoardIsDraw(struct board_t *);

void BoardPrint(struct board_t *);

char BoardInput(void);

void BoardOutput(char *);

#endif
