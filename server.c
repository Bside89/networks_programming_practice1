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
#include "lib/tp1opt.h"
#include "lib/slist.h"

// Flag that indicate if it is time to shutdown (switched by signal handler)
volatile int is_exit = 0;

// Global, to avoid "value escapes local scope" warning;
// Common to all threads / forked processes
fd_set active_sockets;


void* reader_handler(void *arg);

void* writer_handler(void *arg);

void send_unit_message(int sockfd, char *str);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);


int main(int argc, char** argv) {

    if (netopt_set(argc, argv, 1) < 0) { // Get (allocate) all configs by user
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
    char address[SLIST_ADDR_MAX_SIZE], buffer[BUFFER_MAX_SIZE + SLIST_ADDR_MAX_SIZE];

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
    sockfd = socket(AF_INET, netopt_get_transport_protocol(), 0);
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
    serv_addr.sin_port = htons((uint16_t) netopt_get_port());

    // Bind address to socket
    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (netopt_get_transport_protocol() == SOCK_STREAM) { // TCP
        // Listen the socket for incoming connections
        listen(sockfd, 5);
    }

    // Initialize active connection's list
    slist_start((size_t) netopt_get_max_connections_number());

    // Parallelism (fork or new thread)
    if (netopt_get_parallelism_mode() == MULTIPROCESSING_MODE_SET) {
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
            puts("No more inputs. Server went down.");
            break;
        }
        for (i = 0; i < FD_SETSIZE; i++) {

            if (!FD_ISSET(i, &read_sockets)) continue;

            // Accept incoming connections (in TCP)
            if (i == sockfd && netopt_get_transport_protocol() == SOCK_STREAM) {

                newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr,
                                       (unsigned int*) &clilen);
                if (newsockfd < 0) {
                    fprintf(stderr, "ERROR: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                // Fill struct (conn_t) passed as argument in handlers
                memset(&address, 0, sizeof(address));
                sprintf(address, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
                        ntohs(cli_addr.sin_port));

                n = slist_push(newsockfd, address); // Insert into list
                if (n == SLIST_MAX_SIZE_REACHED) {
                    send_unit_message(newsockfd, "Max connections reached. "
                            "Connection refused.\n");
                    continue;
                }

                FD_SET(newsockfd, &active_sockets);

                memset(&buffer, 0, sizeof(buffer));
                sprintf(buffer, "Client %s has logged in.\n", address);
                printf(buffer);
                if (netopt_get_chatmode() == GROUP_MODE_SET)
                    slist_sendall(buffer);

            } else { // Get message from socket (TCP or UDP)

                reader_handler((void*) &i);
            }
        }
    }
    close(sockfd);
    slist_finalize();   // Free allocated memory to slist
    netopt_unset();     // Free allocated memory to netopt

    puts("Bye!");

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

        if (strstr(buffer, "/finalize") != NULL) {
            puts("Closing the server.");
            kill(getppid(), SIGTERM);
        }
        printf("Server >> %s\n", buffer);
    }

    return NULL;
}


void* reader_handler(void *arg) {

    int sockfd = *((int*) arg);
    char msg_buffer[BUFFER_MAX_SIZE], log_buffer[BUFFER_MAX_SIZE +
                                                SLIST_ADDR_MAX_SIZE];
    char *address = slist_get_address_by_socket(sockfd);

    memset(msg_buffer, 0, sizeof(msg_buffer));

    ssize_t rd = read(sockfd, msg_buffer, sizeof(msg_buffer));
    if (rd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (rd == 0) {
        memset(log_buffer, 0, sizeof(msg_buffer));
        sprintf(log_buffer, "Client %s has logged out.\n", address);
        printf(log_buffer);
        if (netopt_get_chatmode() == GROUP_MODE_SET)
            slist_sendall(log_buffer);
        int n = slist_pop(sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(sockfd, &active_sockets);
        return NULL;
    }
    memset(log_buffer, 0, sizeof(msg_buffer));
    sprintf(log_buffer, "[%s]: %s", address, msg_buffer);
    printf(log_buffer);
    if (netopt_get_chatmode() == GROUP_MODE_SET)
        slist_sendall(log_buffer);

    return NULL;
}


void send_unit_message(int sockfd, char *str) {

    ssize_t wd = write(sockfd, str, sizeof(str));
    if (wd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (wd == 0) {
        int n = slist_pop(sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(sockfd, &active_sockets);
    }
}


void sigint_handler(int signum) {
    print_n_close("\nSIGINT received. Exiting.");
}


void sigterm_handler(int signum) {
    print_n_close("\nSIGTERM received. Exiting.");
}


void print_n_close(char *str) {
    puts(str);
    is_exit = 1;
}
