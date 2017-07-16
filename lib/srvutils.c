//
// Created by darkwolf on 15/07/17.
//

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "srvutils.h"


void addr_wrapper(char (*addr)[SLIST_ADDR_MAX_SIZE], struct sockaddr_in addr_in) {

    memset(addr, 0, sizeof(*addr));
    sprintf(*addr, "%s:%hu", inet_ntoa(addr_in.sin_addr),
            ntohs(addr_in.sin_port));

}