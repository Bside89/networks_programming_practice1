//
// Created by darkwolf on 11/07/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
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
    if (new != NULL) {
        new->size = 0;
        new->max_size = size;
        new->conn_array = malloc(size * sizeof(conn_t));
        if (new->conn_array == NULL) {
            free(new);
            return NULL;
        }
        int i;
        for (i = 0; i < size; i++) {
            new->conn_array[i] = closed_connection;
        }
    }
    return new;
}


int slist_push(slist* list, struct connection value) {
    int i;
    if (list->size == list->max_size)
        return SLIST_MAX_SIZE_REACHED;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == NULL_SOCKET) {
            list->conn_array[i] = value;
            list->size++;
            break;
        }
    }
    return SLIST_OK;
}


int slist_pop(slist* list, int sockfd) {
    int i;
    if (list->size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            list->conn_array[i] = closed_connection;
            list->size--;
            close(sockfd); // Close socket
            break;
        }
    }
    return SLIST_OK;
}


int slist_get_by_socket(slist *list, int sockfd, struct connection *value) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            value->sockfd = sockfd;
            strcpy(value->address, list->conn_array[i].address);
            return SLIST_OK;
        }
    }
    return SLIST_ELEMENT_NOT_FOUND;
}


int slist_get_by_address(slist *list, char* addr, struct connection *value) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (strcmp(addr, list->conn_array[i].address) == 0) {
            value->sockfd = list->conn_array[i].sockfd;
            strcpy(value->address, addr);
            return SLIST_OK;
        }
    }
    return SLIST_ELEMENT_NOT_FOUND;
}


unsigned long int slist_size(slist *list) {

    return (list != NULL) ? list->size : 0;
}


void slist_debug(slist *list) {
    int i;
    if (list == NULL) {
        puts("This list is null.");
        return;
    }
    if (list->size == 0) {
        puts("This list is empty.");
        return;
    }
    for (i = 0; i < list->size; i++) {
        if (list->conn_array[i].sockfd != NULL_SOCKET) {
            printf("<%d> <%s>\n", list->conn_array[i].sockfd,
                   list->conn_array[i].address);
        }
    }
}


void slist_destroy(slist** list) {
    int i;
    if (list != NULL && *list != NULL) {
        for (i = 0; i < (*list)->max_size; i++)
            close((*list)->conn_array[i].sockfd); // Close all sockets on list
        free((*list)->conn_array);
        free(*list);
        *list = NULL;
    }
}
