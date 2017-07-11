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
    int tcp_sockfd, tcp_newsockfd, clilen, i = 0, initialization = 0;
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

        slist_push(active_connections, t_args);

        printf("Client %s has logged in.\n", t_args.address);
        printf(">> ACTIVE CONNECTIONS <<\n");
        slist_debug(active_connections);
        printf("List size: %ld\n", slist_size(active_connections));

        // Parallelism (fork or new thread)
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
            slist_pop(active_connections, t_args.sockfd);
            break;
        }
        printf("[%s]: %s", t_args.address, buffer);
        wt = write(t_args.sockfd, "Recebi a mensagem!", 18);
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (wt == 0) {
            printf("Client %s has logged out.\n", t_args.address);
            slist_pop(active_connections, t_args.sockfd);
            break;
        }
    }
    close(t_args.sockfd);
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
    slist_debug(active_connections);
    exit(EXIT_SUCCESS);
}
