#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include "tp1opt.h"

#define LISTEN_ENQ 5
#define BUFFER_MAX_SIZE 256


int sockfd, newsockfd;


void sighandler(int signum);

void* communication_handler(void* arg);

int main(int argc, char** argv) {

    int clilen, i = 0;
    pid_t pid;
    pthread_t threads[LISTEN_ENQ];

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    struct netconfigs options;      // Struct for store program's configs

    signal(SIGINT, sighandler); // Signal handler for CTRL+C

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
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    listen(sockfd, LISTEN_ENQ);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, (unsigned int*) &clilen);
        if (newsockfd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        }
        if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "ERROR forking process.\n");
                exit(1);
            }
            if (pid == 0) { // Child process
                communication_handler((void*) &newsockfd);
                close(sockfd);
                return 0;
            } else {
                close(newsockfd);
            }
        } else {
            if (pthread_create(&(threads[i++]), NULL,
                               communication_handler, (void*) &newsockfd) < 0) {
                fprintf(stderr, "ERROR: %s\n", strerror(errno));
                exit(1);
            }
        }
    }

}


void* communication_handler(void* arg) {

    int sckt = *((int*) arg);
    char buffer[BUFFER_MAX_SIZE];

    memset(buffer, 0, sizeof(buffer));
    if (read(sckt, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }
    printf("Here is the message: %s", buffer);
    if (write(sckt, "Recebi a mensagem!", 18) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }
    close(sckt);
    return NULL;
}


void sighandler(int signum) {
    printf("CTRL+C pressed\n");
    close(newsockfd);
    close(sockfd);
    exit(0);
}