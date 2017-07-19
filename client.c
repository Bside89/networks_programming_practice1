#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "lib/tp1opt.h"
#include "lib/srvutils.h"

#define MSG_BUFFER_SIZE 256


/* **************************************** */
/* Global, to avoid "value escapes local scope" warning;
 * Common to all threads / forked processes */

int sockfd;

/* **************************************** */


/* **************************************** */
/* Functions used */

void* cli_writer(void *arg);

void* cli_reader(void *arg);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);

/* **************************************** */


/**
 * Main program. Start a client for the message application.
 *
 * @param argc see manual usage for details
 * @param argv see manual usage for details
 *
 * @return 0 if success, 1 if error occured
 */
int main(int argc, char** argv) {

    if (netopt_set(argc, argv, 0) < 0) { // Get (allocate) all configs by user
        fprintf(stderr, "Exiting.\n");
        netopt_unset();
        exit(EXIT_FAILURE);
    }

    // Vars initialization
    pid_t pid;
    pthread_t threads;
    struct hostent *server;
    struct sockaddr_in serv_addr, my_addr;
    int len = sizeof(struct sockaddr);

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
    sockfd = socket(AF_INET, netopt_get_transport_protocol(), 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Get host info by IP and port provided
    server = gethostbyname(netopt_get_ip_address());
    if (server == NULL) {
        fprintf(stderr, "Error: No such host.\n");
        exit(EXIT_FAILURE);
    }

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    memcpy((char*) &serv_addr.sin_addr.s_addr, server->h_addr,
           (size_t) server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) netopt_get_port());

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Get my own info (IP and port)
    if (getsockname(sockfd, (struct sockaddr*) &my_addr, (socklen_t*) &len) < 0) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    // Bifurcation (fork or thread)
    if (netopt_get_multiprocessing_mode() == FORK_MODE) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process handles writing
            cli_writer((void*) &my_addr);
        } else {        // Father process handles reading
            cli_reader(NULL);
            kill(pid, SIGTERM);
        }
    } else {
        // New thread handles writing
        if (pthread_create(&threads, NULL, cli_writer, (void*) &my_addr) < 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        cli_reader(NULL); // Main thread handles reading
    }

    netopt_unset();
    close(sockfd);
    sleep(1);

    return EXIT_SUCCESS;
}


void* cli_reader(void *arg) {

    ssize_t rd;
    char buffer[MSG_BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        rd = read(sockfd, buffer, sizeof(buffer) - 1);
        if (rd < 0) {
            perror("read");
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
    struct sockaddr_in my_addr = *((struct sockaddr_in *) arg);
    char* retvalue;
    char msg_buffer[MSG_BUFFER_SIZE], log_buffer[LOG_BUFFER_SIZE];
    char address[SLIST_ADDR_MAX_SIZE];

    addr_wrapper(&address, my_addr);

    while (1) {

        memset(msg_buffer, 0, sizeof(msg_buffer));
        do {
            retvalue = fgets(msg_buffer, sizeof(msg_buffer), stdin);
        } while (retvalue == NULL);

        log_wrapper(&log_buffer, address, msg_buffer);
        printf(log_buffer);

        wt = write(sockfd, log_buffer, sizeof(log_buffer));
        if (wt < 0) {
            perror("write");
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
    sleep(1);
    exit(EXIT_SUCCESS);
}
