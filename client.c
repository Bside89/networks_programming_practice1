#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "tp1opt.h"


int main(int argc, char *argv[]) {

    struct netconfigs options;
    int sockfd;
    int portno;
    char* retvalue;
    char buffer[256];
    struct hostent *server;
    struct sockaddr_in serv_addr;

    set_options(argc, argv, 0, &options);

    // All code here

    //close(sockfd);

    return EXIT_SUCCESS;
}
