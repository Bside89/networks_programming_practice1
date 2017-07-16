//
// Created by darkwolf on 11/07/17.
//

#ifndef TP1_SLIST_H
#define TP1_SLIST_H

#define NULL_SOCKET -1
#define NULL_ADDRESS ""



#define SLIST_OK 0
#define SLIST_MAX_SIZE_REACHED 1
#define SLIST_EMPTY 2
#define SLIST_ALLOCATION_ERROR 4
#define SLIST_ALREADY_SET 5


int slist_start(size_t size);

int slist_push(int sockfd, char* address);

int slist_pop(int sockfd);

char* slist_get_address_by_socket(int sockfd);

int slist_get_socket_by_address(char *address);

unsigned long int slist_size();

int slist_is_full();

int slist_sendall(char *msg, int sender);

void slist_debug();

void slist_finalize();


#endif //TP1_SLIST_H
