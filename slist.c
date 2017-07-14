//
// Created by darkwolf on 11/07/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "slist.h"




typedef struct connection {
    int sockfd;
    char address[SLIST_ADDR_MAX_SIZE];
} conn_t;


typedef struct _slist {
    size_t size;
    size_t max_size;
    conn_t* conn_array;
} slist;


slist *list;

const conn_t closed_connection = {NULL_SOCKET, ""};


int slist_close(int sockfd);


int slist_new(size_t size) {
    list = malloc(sizeof(list));
    if (list != NULL) {
        list->size = 0;
        list->max_size = size;
        list->conn_array = malloc(size * sizeof(conn_t));
        if (list->conn_array == NULL) {
            free(list);
            return SLIST_ALLOCATION_ERROR;
        }
        int i;
        for (i = 0; i < size; i++) {
            list->conn_array[i] = closed_connection;
        }
    }
    return SLIST_OK;
}


int slist_push(int sockfd, char* address) {
    int i;
    if (list->size == list->max_size)
        return SLIST_MAX_SIZE_REACHED;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == NULL_SOCKET) {
            list->conn_array[i].sockfd = sockfd;
            strcpy(list->conn_array[i].address, address);
            list->size++;
            break;
        }
    }
    return SLIST_OK;
}


int slist_pop(int sockfd) {
    int i;
    if (list->size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            list->conn_array[i] = closed_connection;
            list->size--;
            slist_close(sockfd); // Close socket
            break;
        }
    }
    return SLIST_OK;
}


char* slist_get_address_by_socket(int sockfd) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (list->conn_array[i].sockfd == sockfd) {
            return list->conn_array[i].address;
        }
    }
    return NULL_ADDRESS;
}


int slist_get_socket_by_address(char *address) {
    int i;
    for (i = 0; i < list->max_size; i++) {
        if (strcmp(address, list->conn_array[i].address) == 0) {
            return list->conn_array[i].sockfd;
        }
    }
    return NULL_SOCKET;
}


unsigned long int slist_size() {
    return (list != NULL) ? list->size : 0;
}


void slist_debug() {
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


int slist_is_allocated() {
    return list != NULL;
}


int slist_close(int sockfd) {

    ssize_t wt = write(sockfd, "Server shutting down.", 23);
    if (wt <= 0) {
        return 1;
    }
    close(sockfd);
    return 0;
}


void slist_destroy() {
    int i;
    if (slist_is_allocated()) {
        for (i = 0; i < list->max_size; i++)
            slist_close(list->conn_array[i].sockfd); // Close all sockets on list
        free(list->conn_array);
        free(list);
        list = NULL;
    }
}
