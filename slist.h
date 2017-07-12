//
// Created by darkwolf on 11/07/17.
//

#ifndef TP1_SLIST_H
#define TP1_SLIST_H

#define SLIST_ADDR_MAX_SIZE 24 // IP and Port used on print
#define SLIST_OK 0
#define SLIST_MAX_SIZE_REACHED 1
#define SLIST_EMPTY 2
#define SLIST_ELEMENT_NOT_FOUND 3


typedef struct connection {
    int sockfd;
    char address[SLIST_ADDR_MAX_SIZE];
} conn_t;


typedef struct _slist slist;


slist* slist_new(size_t size);

int slist_push(slist* list, struct connection value);

int slist_pop(slist* list, int sockfd);

int slist_get_by_socket(slist *list, int sockfd, struct connection *value);

int slist_get_by_address(slist *list, char* addr, struct connection *value);

unsigned long int slist_size(slist *list);

void slist_debug(slist *list);

void slist_destroy(slist** list);


#endif //TP1_SLIST_H
