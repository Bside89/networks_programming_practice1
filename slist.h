//
// Created by darkwolf on 11/07/17.
//

#ifndef TP1_SLIST_H
#define TP1_SLIST_H

#define NULL_SOCKET -1
#define NULL_ADDRESS ""

#define SLIST_ADDR_MAX_SIZE 24 // IP and Port used on print
#define SLIST_OK 0
#define SLIST_MAX_SIZE_REACHED 1
#define SLIST_EMPTY 2
#define SLIST_ALLOCATION_ERROR 4


int slist_new(size_t size);

int slist_push(int sockfd, char* address);

int slist_pop(int sockfd);

char* slist_get_address_by_socket(int sockfd);

int slist_get_socket_by_address(char *address);

unsigned long int slist_size();

void slist_debug();

int slist_is_allocated();

void slist_destroy();


#endif //TP1_SLIST_H
