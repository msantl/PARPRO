#ifndef __LIST_H
#define __LIST_H

struct list_t {
    int id;

    struct list_t* next;
};

void ListConstruct(struct list_t **);

void ListInsert(struct list_t **, int);

void ListRemove(struct list_t **, int);

void ListDestruct(struct list_t **);

int ListFind(struct list_t **, int);

int ListEmpty(struct list_t **);

int ListHead(struct list_t **);

#endif
