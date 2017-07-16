//
// Created by darkwolf on 15/07/17.
//

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "srvutils.h"


char* addr_wrapper(char (*addr)[SLIST_ADDR_MAX_SIZE], struct sockaddr_in addr_in) {
    memset(addr, 0, sizeof(*addr));
    sprintf(*addr, "%s:%hu", inet_ntoa(addr_in.sin_addr),
            ntohs(addr_in.sin_port));
    return *addr;
}


char* log_wrapper(char (*log)[LOG_BUFFER_SIZE], char *addr, char *message) {
    memset(log, 0, sizeof(*log));
    sprintf(*log, "[%s]: %s", addr, message);
    return *log;
}


char* client_logon_message(char (*log)[LOG_BUFFER_SIZE], char *addr) {
    memset(log, 0, sizeof(*log));
    sprintf(*log, "Client %s has logged in.\n", addr);
    return *log;
}


char* client_logout_message(char (*log)[LOG_BUFFER_SIZE], char *addr) {
    memset(log, 0, sizeof(*log));
    sprintf(*log, "Client %s has logged out.\n", addr);
    return *log;
}
