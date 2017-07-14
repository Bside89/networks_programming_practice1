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

// Flag
volatile int is_exit = 0;

// Global, to avoid "value escapes local scope" warning;
// Common to all threads / forked processes
fd_set active_sockets;
slist* active_clients;


void* reader_handler(void *arg);

void* writer_handler(void *arg);

void send_unit_message(int sockfd, char *str);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);


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
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    int sockfd, newsockfd;
    int clilen = sizeof(cli_addr);
    int i, n;
    conn_t value;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
    sockfd = socket(AF_INET, options.transport_protocol_opt, 0);
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

    if (options.transport_protocol_opt == SOCK_STREAM) { // TCP
        // Listen the socket for incoming connections
        listen(sockfd, options.max_connections_opt);
    }

    // Initialize active connection's list (allocation)
    active_clients = slist_new((size_t) options.max_connections_opt);

    // Parallelism (fork or new thread)
    if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR forking process.\n");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process (for writing handler)
            writer_handler(NULL);
            return 0;
        }
    } else {
        // New thread (for writing handler)
        if (pthread_create(&(threads[0]), NULL, writer_handler, NULL) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // Main loop application (for reading and accept handlers)
    while (!is_exit) {

        read_sockets = active_sockets;

        // I/O multiplexing
        if (select(FD_SETSIZE, &read_sockets, NULL, NULL, NULL) < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }
        for (i = 0; i < FD_SETSIZE; i++) {

            if (!FD_ISSET(i, &read_sockets)) continue;

            // Accept incoming connections (in TCP)
            if (i == sockfd && options.transport_protocol_opt == SOCK_STREAM) {

                newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr,
                                       (unsigned int*) &clilen);
                if (newsockfd < 0) {
                    fprintf(stderr, "ERROR: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                // Fill struct (conn_t) passed as argument in handlers
                memset(&value, 0, sizeof(value));
                value.sockfd = newsockfd;
                sprintf(value.address, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
                        ntohs(cli_addr.sin_port));

                n = slist_push(active_clients, value); // Insert into list
                if (n == SLIST_MAX_SIZE_REACHED) {
                    send_unit_message(newsockfd, "Max connections reached. "
                            "Connection refused.\n");
                    continue;
                }

                FD_SET(newsockfd, &active_sockets);

                printf("Client %s has logged in.\n", value.address);

            } else { // Get message from socket (TCP or UDP)

                n = slist_get_by_socket(active_clients, i, &value);
                assert(n == SLIST_OK);
                reader_handler((void*) &value);

            }
        }
    }
    close(sockfd);
    slist_destroy(&active_clients); // Free allocated memory to slist
    assert(active_clients == NULL);

    return 0;
}


void* writer_handler(void *arg) {

    char buffer[BUFFER_MAX_SIZE];
    char *retvalue;

    while (!is_exit) {
        memset(buffer, 0, sizeof(buffer));
        do {
            retvalue = fgets(buffer, sizeof(buffer), stdin);
        } while (retvalue == NULL);

        if (strstr("finalize", buffer) == NULL) {
            puts("Closing the server.");
            return NULL;
        }
        printf("Server >> %s\n", buffer);
    }

    return NULL;
}


void* reader_handler(void *arg) {

    conn_t connection = *((conn_t*) arg);
    char buffer[BUFFER_MAX_SIZE];

    memset(buffer, 0, sizeof(buffer));

    ssize_t rd = read(connection.sockfd, buffer, sizeof(buffer));
    if (rd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (rd == 0) {
        printf("Client %s has logged out.\n", connection.address);
        int n = slist_pop(active_clients, connection.sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(connection.sockfd, &active_sockets);
        return NULL;
    }
    printf("[%s]: %s", connection.address, buffer);

    return NULL;
}


void send_unit_message(int sockfd, char *str) {

    ssize_t wd = write(sockfd, str, sizeof(str));
    if (wd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (wd == 0) {
        int n = slist_pop(active_clients, sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(sockfd, &active_sockets);
    }
}


void sigint_handler(int signum) {
    print_n_close("SIGINT received. Exiting.\n");
}


void sigterm_handler(int signum) {
    print_n_close("SIGTERM received. Exiting.\n");
}


void print_n_close(char *str) {
    write(STDOUT_FILENO, str, sizeof(str));
    is_exit = 1;
}
