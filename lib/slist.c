//
// Created by darkwolf on 11/07/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "slist.h"
#include "srvutils.h"


typedef struct connection {
    int sockfd;
    char address[SLIST_ADDR_MAX_SIZE];
} conn_t;


typedef struct _slist {
    size_t size;
    size_t max_size;
    conn_t* conn_array;
} slist;


slist list; // All connections will be stored here

int slist_is_set = 0; // Flag indicating if settings are (or not) set


int slist_close(int sockfd);


int slist_start(size_t size) {

    if (slist_is_set != 0)
        return SLIST_ALREADY_SET;
    list.size = 0;
    list.max_size = size;
    list.conn_array = malloc(size * sizeof(conn_t));
    if (list.conn_array == NULL) {
        return SLIST_ALLOCATION_ERROR;
    }
    int i;

    for (i = 0; i < size; i++) {
        list.conn_array[i].sockfd = NULL_SOCKET;
    }
    slist_is_set = 1;
    return SLIST_OK;
}


int slist_push(int sockfd, char* address) {
    int i;
    if (slist_is_full())
        return SLIST_MAX_SIZE_REACHED;
    for (i = 0; i < list.max_size; i++) {
        if (list.conn_array[i].sockfd == NULL_SOCKET) {
            list.conn_array[i].sockfd = sockfd;
            memset(list.conn_array[i].address, 0,
                   sizeof(list.conn_array[i].address));
            strcpy(list.conn_array[i].address, address);
            list.size++;
            break;
        }
    }
    return SLIST_OK;
}


int slist_pop(int sockfd) {
    int i;
    if (list.size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list.max_size; i++) {
        if (list.conn_array[i].sockfd == sockfd) {
            list.conn_array[i].sockfd = NULL_SOCKET;
            list.size--;
            slist_close(sockfd); // Close socket
            break;
        }
    }
    return SLIST_OK;
}


char* slist_get_address_by_socket(int sockfd) {
    int i;
    for (i = 0; i < list.max_size; i++) {
        if (list.conn_array[i].sockfd == sockfd) {
            return list.conn_array[i].address;
        }
    }
    return NULL_ADDRESS;
}


int slist_get_socket_by_address(char *address) {
    int i;
    for (i = 0; i < list.max_size; i++) {
        if (strcmp(address, list.conn_array[i].address) == 0) {
            return list.conn_array[i].sockfd;
        }
    }
    return NULL_SOCKET;
}


unsigned long int slist_size() {
    return (!slist_is_set) ? list.size : 0;
}


int slist_is_full() {
    return list.size == list.max_size;
}


int slist_sendall(char *msg, int sender) {

    int i, sckt;
    char log_buffer[LOG_BUFFER_SIZE];

    memset(&log_buffer, 0, sizeof(log_buffer));
    strcpy(log_buffer, msg);

    for (i = 0; i < list.max_size; i++) {
        sckt = list.conn_array[i].sockfd;
        if (sckt != NULL_SOCKET && sckt != sender) {
            write(list.conn_array[i].sockfd, log_buffer, sizeof(log_buffer));
        }
    }
    return 0;
}


void slist_debug() {
    int i;
    if (!slist_is_set) {
        puts("This list is null.");
        return;
    }
    if (list.size == 0) {
        puts("This list is empty.");
        return;
    }
    for (i = 0; i < list.size; i++) {
        if (list.conn_array[i].sockfd != NULL_SOCKET) {
            printf("<%d> <%s>\n", list.conn_array[i].sockfd,
                   list.conn_array[i].address);
        }
    }
}


int slist_close(int sockfd) {

    ssize_t wt = write(sockfd, "Server is shutting down.", 23);
    if (wt <= 0) {
        return 1;
    }
    close(sockfd);
    return 0;
}


void slist_finalize() {
    int i;
    if (slist_is_set) {
        for (i = 0; i < list.max_size; i++)
            slist_close(list.conn_array[i].sockfd); // Close all sockets on list
        free(list.conn_array);
        slist_is_set = 0;
    }
}
