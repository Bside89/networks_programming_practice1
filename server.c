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
fd_set active_sockets;
slist* active_connections;


void* reader_handler(void *arg);

void* writer_handler(void *arg);

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
    fd_set read_sockets;
    pid_t pid;
    pthread_t threads[2];
    int sockfd, newsockfd, clilen, i, n;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    conn_t value;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize set of sockets
    FD_ZERO(&active_sockets);

    FD_SET(sockfd, &active_sockets); // Insert listen socket to set

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    // Bind address to socket
    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Listen the socket for incoming connections
    listen(sockfd, options.max_connections_opt);
    clilen = sizeof(cli_addr);

    // Initialize active connection's list
    active_connections = slist_new((size_t) options.max_connections_opt);

    // Parallelism (fork or new thread)
    if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR forking process.\n");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process (for writing handler)
            writer_handler(NULL);
        }
    } else {
        // New thread (for writing handler)
        if (pthread_create(&(threads[0]), NULL, writer_handler, NULL) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    while (1) { // Main loop application (for reading and accept handlers)

        read_sockets = active_sockets;

        // I/O multiplexing
        if (select(FD_SETSIZE, &read_sockets, NULL, NULL, NULL) < 0) {
            perror ("Select function.");
            exit (EXIT_FAILURE);
        }
        for (i = 0; i < FD_SETSIZE; i++) {

            if (FD_ISSET(i, &read_sockets)) {

                if (i == sockfd) { // Accept incoming connections

                    newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr,
                                           (unsigned int*) &clilen);
                    if (newsockfd < 0) {
                        fprintf(stderr, "ERROR: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }

                    FD_SET(newsockfd, &active_sockets);

                    // Fill struct (conn_t) passed as argument in handlers
                    memset(&value, 0, sizeof(value));
                    value.sockfd = newsockfd;
                    sprintf(value.address, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
                            ntohs(cli_addr.sin_port));

                    n = slist_push(active_connections, value); // Insert into list
                    assert(n == SLIST_OK);

                    printf("Client %s has logged in.\n", value.address);
                    printf("%s\n", inet_ntoa(cli_addr.sin_addr));
                    printf("%hu\n", ntohs(cli_addr.sin_port));
                    printf(">> ACTIVE CONNECTIONS <<\n");
                    slist_debug(active_connections);
                    printf("List size: %ld\n", slist_size(active_connections));

                } else { // Get (read) message from socket

                    n = slist_get_by_socket(active_connections, i, &value);
                    assert(n == SLIST_OK);
                    reader_handler((void*) &value);

                }
            }
        }
    }
    close(sockfd);
    return 0;
}


void* writer_handler(void *arg) {

    char buffer[BUFFER_MAX_SIZE];
    char *retvalue;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        do {
            retvalue = fgets(buffer, sizeof(buffer), stdin);
        } while (retvalue == NULL);

        if (strstr("finalize", buffer) == NULL) {
            puts("Closing the server.");
            // TODO send signal SIGINT
            return NULL;
        }
        printf("Server >> %s\n", buffer);
        /*
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
        }*/
    }

    return NULL;
}


void* reader_handler(void *arg) {

    conn_t connection = *((conn_t*) arg);
    ssize_t rd;
    char buffer[BUFFER_MAX_SIZE];
    int n;

    memset(buffer, 0, sizeof(buffer));

    rd = read(connection.sockfd, buffer, sizeof(buffer));
    if (rd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (rd == 0) {
        printf("Client %s has logged out.\n", connection.address);
        n = slist_pop(active_connections, connection.sockfd);
        assert(n == SLIST_OK);
        FD_CLR(connection.sockfd, &active_sockets);
        close(connection.sockfd);
        return NULL;
    }
    printf("[%s]: %s", connection.address, buffer);

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
