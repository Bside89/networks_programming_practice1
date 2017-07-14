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
#include "tp1opt.h"

#define BUFFER_MAX_SIZE 256


volatile int is_exit = 0;


void* cli_writer(void *arg);

void* cli_reader(void *arg);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);


struct cli_pthread_args {
    int sockfd;
};


int main(int argc, char** argv) {

    struct netsettings options; // Struct for store program's settings

    // Get all configs by user
    if (set_options(argc, argv, 0, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    pid_t pid;
    pthread_t threads;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    struct cli_pthread_args t_args;

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    server = gethostbyname(options.ip_address);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    // Fill the server (serv_addr) struct
    memset((char*) &serv_addr, 0, sizeof(serv_addr)); // Zero the struct
    memcpy((char*) &serv_addr.sin_addr.s_addr, server->h_addr,
           (size_t) server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) options.connection_port);

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    // Fill struct passed as argument in handlers
    t_args.sockfd = sockfd;

    if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR forking process.\n");
            exit(1);
        }
        if (pid == 0) { // Child process handles writing
            cli_writer((void*) &t_args);
        } else {        // Father process handles reading
            cli_reader((void*) &t_args);
            kill(pid, SIGINT);
        }
    } else {
        // New thread handles writing
        if (pthread_create(&threads, NULL, cli_writer, (void*) &t_args) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        }
        cli_reader((void*) &t_args); // Main thread handles reading
    }

    close(sockfd);

    return 0;
}


void* cli_reader(void *arg) {

    ssize_t rd;
    struct cli_pthread_args t_args = *((struct cli_pthread_args*) arg);
    char buffer[BUFFER_MAX_SIZE];
    while (!is_exit) {
        memset(buffer, 0, sizeof(buffer));
        rd = read(t_args.sockfd, buffer, sizeof(buffer) - 1);
        if (rd  < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        } else if (rd == 0) {
            puts("\nClosing connection.");
            exit(EXIT_SUCCESS);
        }
        printf("[SERVER]: %s\n", buffer);
    }
    return NULL;
}


void* cli_writer(void *arg) {

    ssize_t wt;
    struct cli_pthread_args t_args = *((struct cli_pthread_args*) arg);
    char* retvalue;
    char buffer[BUFFER_MAX_SIZE];

    while (!is_exit) {

        memset(buffer, 0, sizeof(buffer));
        printf(">> ");
        do {
            retvalue = fgets(buffer, sizeof(buffer), stdin);
        } while (retvalue == NULL);

        wt = write(t_args.sockfd, buffer, sizeof(buffer));
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (wt == 0) {
            puts("\nClosing connection.");
            exit(EXIT_SUCCESS);
        }
    }
    return NULL;
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
