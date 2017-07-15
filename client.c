#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include "lib/tp1opt.h"

#define BUFFER_MAX_SIZE 256


int sockfd;


void* cli_writer(void *arg);

void* cli_reader(void *arg);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);


int main(int argc, char** argv) {

    if (netopt_set(argc, argv, 0) < 0) { // Get (allocate) all configs by user
        fprintf(stderr, "Exiting.\n");
        netopt_unset();
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pthread_t threads;
    struct hostent *server;
    struct sockaddr_in serv_addr;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    sockfd = socket(AF_INET, netopt_get_transport_protocol(), 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(netopt_get_ip_address());
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    memcpy((char*) &serv_addr.sin_addr.s_addr, server->h_addr,
           (size_t) server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) netopt_get_port());

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (netopt_get_parallelism_mode() == MULTIPROCESSING_MODE_SET) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR forking process.\n");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process handles writing
            cli_writer((void*) &sockfd);
        } else {        // Father process handles reading
            cli_reader((void*) &sockfd);
            kill(pid, SIGTERM);
        }
    } else {
        // New thread handles writing
        if (pthread_create(&threads, NULL, cli_writer, (void*) &sockfd) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        cli_reader((void*) &sockfd); // Main thread handles reading
    }

    netopt_unset();
    close(sockfd);

    return 0;
}


void* cli_reader(void *arg) {

    ssize_t rd;
    int sckt = *((int*) arg);
    char buffer[BUFFER_MAX_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        rd = read(sckt, buffer, sizeof(buffer) - 1);
        if (rd  < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            break;
        } else if (rd == 0) {
            puts("\nClosing connection on reader.");
            break;
        }
        printf(buffer);
    }
    return NULL;
}


void* cli_writer(void *arg) {

    ssize_t wt;
    int sckt = *((int*) arg);
    char* retvalue;
    char buffer[BUFFER_MAX_SIZE];

    while (1) {

        memset(buffer, 0, sizeof(buffer));
        do {
            retvalue = fgets(buffer, sizeof(buffer), stdin);
        } while (retvalue == NULL);

        wt = write(sckt, buffer, sizeof(buffer));
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            break;
        } else if (wt == 0) {
            puts("\nClosing connection on writer.");
            break;
        }
    }
    return NULL;
}


void sigint_handler(int signum) {
    print_n_close("\nSIGINT received. Exiting.");
}


void sigterm_handler(int signum) {
    print_n_close("\nSIGTERM received. Exiting.");
}


void print_n_close(char *str) {
    puts(str);
    close(sockfd);
    exit(EXIT_SUCCESS);
}
