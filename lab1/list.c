#include "list.h"

#include <stdlib.h>

void ListConstruct(struct list_t **head) {
    *head = NULL;
    return;
}

void ListInsert(struct list_t **head, int id) {

    if (*head == NULL) {
        struct list_t *element = (struct list_t *) malloc(sizeof(struct list_t));

        element->id = id;
        element->next = NULL;

        *head = element;
    } else if ((*head)->next == NULL) {
        struct list_t *element = (struct list_t *) malloc(sizeof(struct list_t));

        element->id = id;
        element->next = NULL;

        (*head)->next = element;
    } else {
        ListInsert(&(*head)->next, id);
    }

    return;
}

void ListRemove(struct list_t **head, int id) {
    struct list_t *curr, *prev = NULL;

    if (*head == NULL) {
        return;
    }

    for (curr = *head; curr; curr = curr->next) {

        if (curr->id == id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                *head = curr->next;
            }

            free(curr);
            break;
        }

        prev = curr;
    }

    return;
}

void ListDestruct(struct list_t **head) {
    struct list_t *curr, *temp;

    if (*head == NULL) {
        return;
    }

    for (curr = *head; curr;) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }

    *head = NULL;

    return;
}

int ListFind(struct list_t **head, int id) {
    struct list_t *curr;

    for(curr = *head; curr; curr = curr->next) {
        if (curr->id == id) return 1;
    }

    return 0;
}
