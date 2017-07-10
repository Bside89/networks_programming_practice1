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

#define BUFFER_MAX_SIZE 256


int sockfd, newsockfd;


void sighandler(int signum);

void* communication_handler(void* arg);


struct srv_pthread_args {
    int sockfd;
};


int main(int argc, char** argv) {

    struct netconfigs options; // Struct for store program's configs

    // Get all configs by user
    if (set_options(argc, argv, 1, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pthread_t threads[options.max_connections_opt];
    int clilen, i = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    struct srv_pthread_args t_args;

    signal(SIGINT, sighandler); // Signal handler for CTRL+C

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

    listen(sockfd, options.max_connections_opt);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr,
                           (unsigned int*) &clilen);
        if (newsockfd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        }

        // Fill struct passed as argument in handlers
        t_args.sockfd = newsockfd;

        if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "ERROR forking process.\n");
                exit(1);
            }
            if (pid == 0) { // Child process
                communication_handler((void*) &t_args);
                break;
            } else {
                close(newsockfd);
            }
        } else {
            if (pthread_create(&(threads[i++]), NULL,
                               communication_handler, (void*) &t_args) < 0) {
                fprintf(stderr, "ERROR: %s\n", strerror(errno));
                exit(1);
            }
        }
    }
    close(sockfd);
    return 0;
}


void* communication_handler(void* arg) {

    ssize_t rd, wt;
    struct srv_pthread_args t_args = *((struct srv_pthread_args*) arg);
    char buffer[BUFFER_MAX_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        rd = read(t_args.sockfd, buffer, sizeof(buffer));
        if (rd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        } else if (rd == 0) {
            puts("ALERT: Closing a connection.");
            break;
        }
        printf("Here is the message: %s", buffer);
        wt = write(t_args.sockfd, "Recebi a mensagem!", 18);
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        } else if (wt == 0) {
            puts("ALERT: Closing a connection.");
            break;
        }
    }
    close(t_args.sockfd);
    return NULL;
}


void sighandler(int signum) {
    printf("CTRL+C pressed\n");
    close(newsockfd);
    close(sockfd);
    exit(0);
}