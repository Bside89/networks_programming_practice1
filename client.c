#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include "tp1opt.h"


int sockfd;


void sighandler(int signum) {
    printf("\nCTRL+C pressed\n");
    exit(0);
}


int main(int argc, char** argv) {

    char* retvalue;
    char buffer[256];

    struct hostent *server;
    struct sockaddr_in serv_addr;
    struct netconfigs options;      // Struct for store program's configs

    signal(SIGINT, sighandler); // Signal handler for CTRL+C

    // Get all configs by user
    if (set_options(argc, argv, 0, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    server = gethostbyname(options.ip_address);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    memcpy((char*) &serv_addr.sin_addr.s_addr, server->h_addr,
           (size_t) server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    printf("Please enter the message: ");
    memset(buffer, 0, sizeof(buffer));

    do {
        retvalue = fgets(buffer, sizeof(buffer), stdin);
    } while (retvalue == NULL);

    if (write(sockfd, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    memset(buffer, 0, sizeof(buffer));
    if (read(sockfd, buffer, sizeof(buffer) - 1) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    printf("%s\n", buffer);

    close(sockfd);

    return 0;
}