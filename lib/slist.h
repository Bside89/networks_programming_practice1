//
// Created by darkwolf on 11/07/17.
//

#ifndef TP1_SLIST_H
#define TP1_SLIST_H

#include <netinet/in.h>
#include "srvutils.h"

#define NULL_SOCKET -1
#define NULL_ADDRESS ""

#define SLIST_UDP_MODE 0
#define SLIST_TCP_MODE 1

#define SLIST_OK 0
#define SLIST_MAX_SIZE_REACHED 1
#define SLIST_EMPTY 2
#define SLITS_ELEMENT_ALREADY_EXISTS 3
#define SLIST_ALLOCATION_ERROR 4
#define SLIST_ALREADY_SET 5
#define SLIST_INVALID_ELEMENT 6


typedef struct connection {
    int sockfd;
    struct sockaddr_in info;
    int infolen;
    char address[SLIST_ADDR_MAX_SIZE];
} conn_t;


int slist_start(size_t size, int mode);

int slist_push(int sockfd, struct sockaddr_in info);

int slist_pop(int sockfd);

int slist_isset_by_sockaddr(struct sockaddr_in client);

char* slist_get_address_by_socket(int sockfd);

int slist_get_socket_by_address(char *address);

conn_t* slist_get_by_address(char* address);

unsigned long int slist_size();

int slist_is_full();

int slist_sendall(char *msg, int sender, struct sockaddr_in sender_info);

void slist_debug();

void slist_finalize();


#endif //TP1_SLIST_H
