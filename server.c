#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "tp1opt.h"
#include "slist.h"

#define BUFFER_MAX_SIZE 256
#define ADDR_STR_MAX_SIZE 24 // IP and Port used on print


int tcp_sockfd, tcp_newsockfd;


void* communication_handler(void* arg);


int main(int argc, char** argv) {

    struct netsettings options; // Struct for store program's settings

    // Get all configs by user
    if (set_options(argc, argv, 1, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pthread_t threads[options.max_connections_opt + 1];
    int clilen, i = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    conn_t t_args;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    if (bind(tcp_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    listen(tcp_sockfd, options.max_connections_opt);
    clilen = sizeof(cli_addr);

    while (1) {
        tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr*) &cli_addr,
                           (unsigned int*) &clilen);
        if (tcp_newsockfd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Fill struct passed as argument in handlers
        memset(&t_args, 0, sizeof(t_args));
        t_args.sockfd = tcp_newsockfd;
        sprintf(t_args.address, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
                cli_addr.sin_port);
        printf("Client %s has logged in.\n", t_args.address);

        if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "ERROR forking process.\n");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) { // Child process
                communication_handler((void*) &t_args);
                break;
            } else {
                close(tcp_newsockfd);
            }
        } else {
            if (pthread_create(&(threads[i++]), NULL,
                               communication_handler, (void*) &t_args) < 0) {
                fprintf(stderr, "ERROR: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }
    close(tcp_sockfd);
    return 0;
}


void* communication_handler(void* arg) {

    ssize_t rd, wt;
    struct connection t_args = *((struct connection*) arg);
    char buffer[BUFFER_MAX_SIZE];

    while (1) {

        memset(buffer, 0, sizeof(buffer));
        rd = read(t_args.sockfd, buffer, sizeof(buffer));
        if (rd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (rd == 0) {
            printf("Client %s has logged out.\n", t_args.address);
            break;
        }
        printf("[%s]: %s", t_args.address, buffer);
        wt = write(t_args.sockfd, "Recebi a mensagem!", 18);
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (wt == 0) {
            printf("Client %s has logged out.\n", t_args.address);
            break;
        }
    }
    close(t_args.sockfd);
    return NULL;
}
