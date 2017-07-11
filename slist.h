//
// Created by darkwolf on 11/07/17.
//

#ifndef TP1_SLIST_H
#define TP1_SLIST_H

#define ADDR_STR_MAX_SIZE 24 // IP and Port used on print


typedef struct connection {
    int sockfd;
    char address[ADDR_STR_MAX_SIZE];
} conn_t;


typedef struct _slist slist;


slist* slist_new(size_t size);

void slist_push(slist* list, struct connection value);

void slist_pop(slist* list, int sockfd);

void slist_get(slist *list, int sockfd, struct connection *value);

void slist_destroy(slist** list);


#endif //TP1_SLIST_H
