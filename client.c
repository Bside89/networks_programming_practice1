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


int tcp_sockfd;


void sighandler(int signum);

void* cli_writer(void *arg);

void* cli_reader(void *arg);


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

    pid_t pid;
    pthread_t threads[2];
    struct hostent *server;
    struct sockaddr_in serv_addr;
    struct cli_pthread_args t_args;

    signal(SIGINT, sighandler); // Signal handler for CTRL+C

    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
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

    if (connect(tcp_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        exit(1);
    }

    // Fill struct passed as argument in handlers
    t_args.sockfd = tcp_sockfd;

    if (options.parallelism_mode_opt == MULTIPROCESSING_MODE_SET) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR forking process.\n");
            exit(1);
        }
        if (pid == 0) { // Child process
            cli_writer((void*) &t_args);
        } else {
            cli_reader((void*) &t_args);
        }
    } else {
        if (pthread_create(&(threads[0]), NULL,
                           cli_writer, (void*) &t_args) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        }
        if (pthread_create(&(threads[1]), NULL,
                           cli_reader, (void*) &t_args) < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        }
        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
    }

    close(tcp_sockfd);

    return 0;
}


void sighandler(int signum) {
    printf("\nCTRL+C pressed\n");
    close(tcp_sockfd);
    exit(0);
}


void* cli_reader(void *arg) {

    ssize_t rd;
    struct cli_pthread_args t_args = *((struct cli_pthread_args*) arg);
    char buffer[BUFFER_MAX_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        rd = read(t_args.sockfd, buffer, sizeof(buffer) - 1);
        if (rd  < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        } else if (rd == 0) {
            puts("Closing connection");
            break;
        }
        printf("Server >> %s\n", buffer);
    }
    return NULL;
}


void* cli_writer(void *arg) {

    ssize_t wt;
    struct cli_pthread_args t_args = *((struct cli_pthread_args*) arg);
    char* retvalue;
    char buffer[BUFFER_MAX_SIZE];

    puts("Digite suas mensagens abaixo:");
    while (1) {

        memset(buffer, 0, sizeof(buffer));
        do {
            retvalue = fgets(buffer, sizeof(buffer), stdin);
        } while (retvalue == NULL);

        wt = write(t_args.sockfd, buffer, sizeof(buffer));
        if (wt < 0) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            exit(1);
        } else if (wt == 0) {
            puts("Closing connection");
            break;
        }
    }
    return NULL;
}
