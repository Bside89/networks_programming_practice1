//
// Created by darkwolf on 11/07/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "slist.h"


typedef struct _slist {
    size_t size;
    size_t max_size;
    conn_t* conn_array;
} slist;


slist list; // All connections will be stored here

int slist_mode = -1; // Protocol (TCP or UDP), -1 for not set

int slist_is_set = 0; // Flag indicating if settings are (or not) set


int slist_close(int sockfd);


int slist_start(size_t size, int mode) {

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
    slist_mode = (mode) ? SLIST_TCP_MODE : SLIST_UDP_MODE;
    slist_is_set = 1;
    return SLIST_OK;
}


int slist_push(int sockfd, struct sockaddr_in info) {
    int i;
    if (slist_is_full())
        return SLIST_MAX_SIZE_REACHED;
    if (slist_isset_by_sockaddr(info))
        return SLITS_ELEMENT_ALREADY_EXISTS;
    if (info.sin_port == 0)
        return SLIST_INVALID_ELEMENT;
    for (i = 0; i < list.max_size; i++) {
        if (list.conn_array[i].sockfd == NULL_SOCKET) {

            list.conn_array[i].sockfd = sockfd;
            memcpy(&list.conn_array[i].info, &info, sizeof(info));
            memset(list.conn_array[i].address, 0,
                   sizeof(list.conn_array[i].address));
            addr_wrapper(&list.conn_array[i].address, info);
            list.conn_array[i].infolen = sizeof(list.conn_array[i].info);

            list.size++;
            break;
        }
    }
    return SLIST_OK;
}


int slist_isset_by_sockaddr(struct sockaddr_in client) {
    int i;
    struct sockaddr_in other;
    for (i = 0; i < list.max_size; i++) {
        other = list.conn_array[i].info;
        if (other.sin_addr.s_addr == client.sin_addr.s_addr &&
                other.sin_port == client.sin_port) {
            return 1;
        }
    }
    return 0;
}


int slist_pop(int sockfd) {
    int i;
    if (list.size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list.max_size; i++) {
        if (list.conn_array[i].sockfd == sockfd) {
            list.conn_array[i].sockfd = NULL_SOCKET;
            list.size--;
            if (slist_mode == SLIST_TCP_MODE)
                slist_close(sockfd); // Close socket (only TCP)
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
    conn_t *rec = slist_get_by_address(address);
    if (rec == NULL)
        return NULL_SOCKET;
    return rec->sockfd;
}


conn_t* slist_get_by_address(char* address) {
    int i;
    for (i = 0; i < list.max_size; i++) {
        if (strcmp(address, list.conn_array[i].address) == 0) {
            return &list.conn_array[i];
        }
    }
    return NULL;
}


unsigned long int slist_size() {
    return (!slist_is_set) ? list.size : 0;
}


int slist_is_full() {
    return list.size == list.max_size;
}


int slist_sendall(char *msg, int sender, struct sockaddr_in sender_info) {

    int i;
    char log_buffer[LOG_BUFFER_SIZE];
    conn_t *c;

    memset(&log_buffer, 0, sizeof(log_buffer));
    strcpy(log_buffer, msg);

    for (i = 0; i < list.max_size; i++) {

        c = &list.conn_array[i];

        if (c->sockfd == NULL_SOCKET)
            continue;

        if (slist_mode == SLIST_TCP_MODE) { // TCP Protocol
            if (c->sockfd != sender) {
                write(c->sockfd, log_buffer, sizeof(log_buffer));
            }
        } else {                            // UDP Protocol
            if (c->info.sin_addr.s_addr != sender_info.sin_addr.s_addr ||
                    c->info.sin_port != sender_info.sin_port) {
                sendto(c->sockfd, log_buffer, sizeof(log_buffer), 0,
                       (struct sockaddr *) &c->info,
                       (socklen_t) c->infolen);
            }
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
            printf("<Socket: %d> <Addr_str: %s> <Addr: %s> <Port: %hu>\n",
                   list.conn_array[i].sockfd, list.conn_array[i].address,
                   inet_ntoa(list.conn_array[i].info.sin_addr),
                   ntohs(list.conn_array[i].info.sin_port));
        }
    }
}


int slist_close(int sockfd) {

    if (slist_mode == SLIST_TCP_MODE)
        return -1;
    ssize_t wt = write(sockfd, "Server is shutting down.", 23);
    if (wt <= 0)
        return 1;
    close(sockfd);
    return 0;
}


void slist_finalize() {
    int i;
    if (slist_is_set) {
        if (slist_mode == SLIST_TCP_MODE) {
            for (i = 0; i < list.max_size; i++)
                slist_close(list.conn_array[i].sockfd); // Close all sockets
        }
        free(list.conn_array);
        slist_is_set = 0;
        slist_mode = -1;
    }
}
