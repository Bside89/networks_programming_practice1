#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "tp1opt.h"


int main(int argc, char *argv[]) {

    struct netconfigs options;
    int sockfd;
    int newsockfd;
    int portno;
    int clilen;
    char buffer[256];
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    set_options(argc, argv, 1, &options);

    // All code here

    //close(newsockfd);
    //close(sockfd);

    return EXIT_SUCCESS;
}