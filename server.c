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
#include <assert.h>
#include "tp1opt.h"
#include "slist.h"

#define BUFFER_MAX_SIZE 256


// Global, to avoid "value escapes local scope" warning;
// Common to all threads / forked processes
fd_set sockets;
slist* active_connections;


void* communication_handler(void* arg);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_exit(char *str);


int main(int argc, char** argv) {

    struct netsettings options; // Struct for store program's settings

    // Get all configs by user
    if (set_options(argc, argv, 1, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Vars initialization
    pid_t pid;
    pthread_t threads[options.max_connections_opt + 1];
    int tcp_sockfd, tcp_newsockfd, clilen, i = 0, n, initialization = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    conn_t t_args;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
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

    // Bind address to socket
    if (bind(tcp_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Listen the socket for incoming connections
    listen(tcp_sockfd, options.max_connections_opt);
    clilen = sizeof(cli_addr);

    // Initialize set of sockets and list of active connections;
    FD_ZERO(&sockets);
    active_connections = slist_new((size_t) options.max_connections_opt);

    // Main loop application
    while (1) {
        // Accept incoming connections
        tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr*) &cli_addr,
                           (unsigned int*) &clilen);
        if (tcp_newsockfd < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        FD_SET(tcp_newsockfd, &sockets);

        // Fill struct (conn_t) passed as argument in handlers
        memset(&t_args, 0, sizeof(t_args));
        t_args.sockfd = tcp_newsockfd;
        sprintf(t_args.address, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
                cli_addr.sin_port);


        n = slist_push(active_connections, t_args);
        assert(n == SLIST_OK);

        printf("Client %s has logged in.\n", t_args.address);
        printf("%s\n", inet_ntoa(cli_addr.sin_addr));
        printf("%hu\n", cli_addr.sin_port);
        printf(">> ACTIVE CONNECTIONS <<\n");
        slist_debug(active_connections);
        printf("List size: %ld\n", slist_size(active_connections));

        if (initialization == 0) {

            // Parallelism (fork or new thread)
            if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
                pid = fork();
                if (pid < 0) {
                    fprintf(stderr, "ERROR forking process.\n");
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) { // Child process
                    communication_handler(NULL);
                    break;
                }
            } else {
                if (pthread_create(&(threads[i++]), NULL,
                                   communication_handler, NULL) < 0) {
                    fprintf(stderr, "ERROR: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }

            initialization = 1;
        }

    }
    close(tcp_sockfd);
    return 0;
}


void* communication_handler(void* arg) {

    ssize_t rd, wt;
    conn_t value;
    fd_set read_sockets;
    char buffer[BUFFER_MAX_SIZE];
    int i, n;

    while (1) {

        read_sockets = sockets;

        // I/O multiplexing
        if (select(FD_SETSIZE, &read_sockets, NULL, NULL, NULL) < 0) {
            perror ("Select function.");
            exit (EXIT_FAILURE);
        }
        for (i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_sockets)) {
                memset(buffer, 0, sizeof(buffer));
                n = slist_get_element(active_connections, i, &value);
                assert(n == SLIST_OK);
                rd = read(i, buffer, sizeof(buffer));
                if (rd < 0) {
                    fprintf(stderr, "ERROR: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                } else if (rd == 0) {
                    printf("Client %s has logged out.\n", value.address);
                    n = slist_pop(active_connections, i);
                    assert(n == SLIST_OK);
                    FD_CLR(i, &sockets);
                    close(i);
                    continue;
                }
                printf("[%s]: %s", value.address, buffer);
                wt = write(i, "Recebi a mensagem!", 18);
                if (wt < 0) {
                    fprintf(stderr, "ERROR: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                } else if (wt == 0) {
                    printf("Client %s has logged out.\n", value.address);
                    n = slist_pop(active_connections, i);
                    assert(n == SLIST_OK);
                    FD_CLR(i, &sockets);
                    close(i);
                    continue;
                }
            }
        }
    }
    return NULL;
}


void sigint_handler(int signum) {
    print_n_exit("SIGINT received. Exiting.");
}


void sigterm_handler(int signum) {
    print_n_exit("SIGTERM received. Exiting.");
}


void print_n_exit(char *str) {
    puts(str);
    slist_destroy(&active_connections);
    assert(active_connections == NULL);
    exit(EXIT_SUCCESS);
}
