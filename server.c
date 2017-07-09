#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "tp1opt.h"

#define LISTEN_ENQ 5

int main(int argc, char** argv) {

    int sockfd;
    int newsockfd;
    int clilen;
    char buffer[256];

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    struct netconfigs options;      // Struct for store program's configs

    // Get all configs by user
    if (set_options(argc, argv, 1, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct

    //portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    listen(sockfd, LISTEN_ENQ);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, (unsigned int*) &clilen);
    if (newsockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }
    memset(buffer, 0, sizeof(buffer));

    if (read(newsockfd, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    printf("Here is the message: %s", buffer);

    if (write(newsockfd, "Recebi a mensagem!", 18) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    close(newsockfd);
    close(sockfd);

    return 0;
}