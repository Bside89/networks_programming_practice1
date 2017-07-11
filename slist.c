//
// Created by darkwolf on 11/07/17.
//

#include <stdlib.h>
#include <string.h>
#include "slist.h"

#define NULL_SOCKET -1


struct _slist {
    size_t size;
    size_t max_size;
    conn_t* conn_array;
};


const conn_t closed_connection = {NULL_SOCKET, ""};


slist* slist_new(size_t size) {
    slist *new = malloc(sizeof(new));
    new->size = 0;
    new->max_size = size;
    new->conn_array = malloc(size * sizeof(conn_t));
    int i;
    for (i = 0; i < size; i++) {
        new->conn_array[i] = closed_connection;
    }
    return new;
}


void slist_push(slist* list, struct connection value) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == NULL_SOCKET) {
            list->conn_array[i] = value;
            list->size++;
            return;
        }
    }
}


void slist_pop(slist* list, int sockfd) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            list->conn_array[i] = closed_connection;
            list->size--;
            return;
        }
    }
}


void slist_get(slist *list, int sockfd, struct connection *value) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            value->sockfd = sockfd;
            strcpy(value->address, list->conn_array[i].address);
            return;
        }
    }
}


void slist_destroy(slist** list) {

    free((*list)->conn_array);
    free(*list);
    *list = NULL;
}
