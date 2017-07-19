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


int slist_close(int sockfd, struct sockaddr_in *client, socklen_t clilen);


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


int slist_pop_by_socket(int sockfd) {
    int i;
    conn_t *c;
    if (list.size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list.max_size; i++) {
        c = &list.conn_array[i];
        if (c->sockfd == sockfd) {
            c->sockfd = NULL_SOCKET;
            list.size--;
            slist_close(c->sockfd, &c->info, (socklen_t) c->infolen);
            break;
        }
    }
    return SLIST_OK;
}


int slist_pop_by_addr(struct sockaddr_in client) {
    int i;
    conn_t *c;
    if (list.size == 0)
        return SLIST_EMPTY;
    for (i = 0; i < list.max_size; i++) {
        c = &list.conn_array[i];
        if (c->info.sin_addr.s_addr == client.sin_addr.s_addr &&
                c->info.sin_port == client.sin_port) {
            c->sockfd = NULL_SOCKET;
            list.size--;
            slist_close(c->sockfd, &c->info, (socklen_t) c->infolen);
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
    ssize_t st;

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
                st = sendto(c->sockfd, log_buffer, sizeof(log_buffer), 0,
                       (struct sockaddr *) &c->info,
                       (socklen_t) c->infolen);
                if (st == 0) { // Client no more active
                    slist_pop_by_addr(c->info);
                }
            }
        }
    }
    return 0;
}


void slist_debug() {

    int i;
    puts("***************");
    puts("SLIST'S SETTINGS");
    puts("***************");
    if (!slist_is_set) {
        puts("This list is null.");
        return;
    }
    if (list.size == 0) {
        puts("This list is empty.");
        return;
    }
    printf("Transport protocol used: %s\n",
           slist_mode == SLIST_TCP_MODE ? "TCP" : "UDP");
    puts("Active clients:");
    for (i = 0; i < list.size; i++) {
        if (list.conn_array[i].sockfd != NULL_SOCKET) {
            printf("<Socket: %d> <Addr_str: %s> <Addr: %s> <Port: %hu>\n",
                   list.conn_array[i].sockfd, list.conn_array[i].address,
                   inet_ntoa(list.conn_array[i].info.sin_addr),
                   ntohs(list.conn_array[i].info.sin_port));
        }
    }
    puts("***************");
}


int slist_close(int sockfd, struct sockaddr_in *client, socklen_t clilen) {

    const char *msg = "Server is shutting down.";

    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *) client, clilen);
    if (slist_mode == SLIST_TCP_MODE)
        close(sockfd); // Close socket only in TCP
    return 0;
}


void slist_finalize() {
    int i;
    conn_t *c;
    if (!slist_is_set)
        return;
    for (i = 0; i < list.max_size; i++) {
        c = &list.conn_array[i];
        if (c->sockfd != NULL_SOCKET)
            // Close all sockets
            slist_close(c->sockfd, &c->info, (socklen_t) c->infolen);
    }
    free(list.conn_array);
    slist_is_set = 0;
    slist_mode = -1;
}
