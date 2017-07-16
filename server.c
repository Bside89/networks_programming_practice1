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
#include "lib/srvutils.h"


// Flag that indicate if it is time to shutdown (switched by signal handler)
volatile int is_exit = 0;


/* **************************************** */
/* Global, to avoid "value escapes local scope" warning;
 * Common to all threads / forked processes */

fd_set active_sockets;

int reader_writer_pipe[2];

/* **************************************** */


/* **************************************** */
/* Functions used */

void* reader_handler(void *arg);

void* writer_handler(void *arg);

void accept_connections_handler(int listen_socket);

void send_unit_message(int sockfd, char *str);

int locate_addressee(char *srvaddr, char (*msgbuffer)[LOG_BUFFER_SIZE],
                       char (*address)[SLIST_ADDR_MAX_SIZE]);

void sigint_handler(int signum);

void sigterm_handler(int signum);

void print_n_close(char *str);

/* **************************************** */


/**
 * Main program. Start a server for the message application.
 *
 * @param argc see manual usage for details
 * @param argv see manual usage for details
 *
 * @return 0 if success, 1 if error occured
 */
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
    int sockfd;
    int i;
    char server_address[SLIST_ADDR_MAX_SIZE];

    signal(SIGINT, sigint_handler);     // Signal handler for SIGINT
    signal(SIGTERM, sigterm_handler);   // Signal handler for SIGTERM

    // Get socket from system
    sockfd = socket(AF_INET, netopt_get_transport_protocol(), 0);
    if (sockfd < 0) {
        perror("socket");
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
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Fill server string to the correct format message
    addr_wrapper(&server_address, serv_addr);

    if (netopt_get_transport_protocol() == SOCK_STREAM) { // TCP
        // Listen the socket for incoming connections
        listen(sockfd, 5);
    }

    // Initialize active connection's list
    slist_start((size_t) netopt_get_max_connections_number());

    // Initialize pipe communication between reader and writer
    if (pipe(reader_writer_pipe) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Bifurcation (fork or thread)
    if (netopt_get_parallelism_mode() == MULTIPROCESSING_MODE) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process (for writing handler)
            close(reader_writer_pipe[1]); // Writer handler will not write nothing
            writer_handler((void*) server_address);
            return 0;
        } else {
            close(reader_writer_pipe[0]); // Reader handler will not read nothing
        }
    } else {
        // New thread (for writing handler)
        if (pthread_create(&(threads[0]), NULL, writer_handler,
                           (void*) server_address) < 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Main loop application (for reading and accept handlers)
    while (!is_exit) {

        read_sockets = active_sockets;

        // I/O multiplexing
        if (select(FD_SETSIZE, &read_sockets, NULL, NULL, NULL) < 0) {
            puts("No more inputs. Server is shutting down.");
            break;
        }
        for (i = 0; i < FD_SETSIZE; i++) {

            if (!FD_ISSET(i, &read_sockets))
                continue;
            // Accept incoming connections (in TCP)
            if (i == sockfd && netopt_get_transport_protocol() == SOCK_STREAM) {
                accept_connections_handler(sockfd);
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


void accept_connections_handler(int listen_socket) {

    char address[SLIST_ADDR_MAX_SIZE];
    char log_buffer[LOG_BUFFER_SIZE];
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    int newsockfd = accept(listen_socket, (struct sockaddr*) &cli_addr,
                       (unsigned int*) &clilen);
    if (newsockfd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Fill data to insert in connection's list
    addr_wrapper(&address, cli_addr);

    // Insert into list
    if (slist_push(newsockfd, address) == SLIST_MAX_SIZE_REACHED) {
        send_unit_message(newsockfd, "Max connections reached. "
                "Connection refused.\n");
        return;
    }

    FD_SET(newsockfd, &active_sockets);

    // Fill log_buffer to format message
    printf(client_logon_message(&log_buffer, address));

    if (netopt_get_chatmode() == GROUP_MODE) // Send message to writer
        write(reader_writer_pipe[1], log_buffer, sizeof(log_buffer));

}


void* writer_handler(void *arg) {

    char log_buffer[LOG_BUFFER_SIZE];
    char addr[SLIST_ADDR_MAX_SIZE];
    char *retvalue;
    char *server_address = (char*) arg;
    int sockfd;

    while (!is_exit) {

        memset(log_buffer, 0, sizeof(log_buffer));

        if (netopt_get_chatmode() == UNIQUE_MODE) {
            do {
                retvalue = fgets(log_buffer, sizeof(log_buffer), stdin);
            } while (retvalue == NULL);
            if (locate_addressee(server_address, &log_buffer, &addr) < 0) {
                puts("Message not send due to incorrect format.");
                continue;
            }
            sockfd = slist_get_socket_by_address(addr);
            if (sockfd == NULL_SOCKET) {
                puts("Client not found or not connected (error on get socket).");
                continue;
            }
            ssize_t wt = write(sockfd, log_buffer, sizeof(log_buffer));
            if (wt < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            } else if (wt == 0) {
                puts("Client not found or not connected (error on write).");
                continue;
            }
            printf(log_buffer);

        } else { // Receive message from reader
            
            if (read(reader_writer_pipe[0], log_buffer, sizeof(log_buffer)) > 0)
                slist_sendall(log_buffer);
        }
    }
    return NULL;
}


void* reader_handler(void *arg) {

    int sockfd = *((int*) arg);
    char log_buffer[LOG_BUFFER_SIZE];
    char *address = slist_get_address_by_socket(sockfd);

    memset(log_buffer, 0, sizeof(log_buffer));

    ssize_t rd = read(sockfd, log_buffer, sizeof(log_buffer));
    if (rd < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    } else if (rd == 0) { // Client has logged out
        int n = slist_pop(sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(sockfd, &active_sockets);
        client_logout_message(&log_buffer, address);
    }
    printf(log_buffer);
    if (netopt_get_chatmode() == GROUP_MODE)
        write(reader_writer_pipe[1], log_buffer, sizeof(log_buffer));

    return NULL;
}


void send_unit_message(int sockfd, char *str) {

    ssize_t wd = write(sockfd, str, sizeof(str));
    if (wd < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    } else if (wd == 0) {
        int n = slist_pop(sockfd); // Close occurs here
        assert(n == SLIST_OK);
        FD_CLR(sockfd, &active_sockets);
    }
}


int locate_addressee(char *srvaddr, char (*msgbuffer)[LOG_BUFFER_SIZE],
                       char (*address)[SLIST_ADDR_MAX_SIZE]) {

    char *init = NULL, *end = NULL;
    char space = ' ';
    char localbuffer[LOG_BUFFER_SIZE];

    if (strstr(*msgbuffer, ":~$ ") != *msgbuffer)
        return -1;

    memset(localbuffer, 0, sizeof(localbuffer));
    memset(address, 0, sizeof(*address));
    strcpy(localbuffer, *msgbuffer);

    init = strchr(localbuffer, space);
    if (init == NULL)
        return -1;
    end = strchr(++init, space);
    if (end == NULL)
        return -1;
    size_t addrsize = strlen(init) - strlen(++end) - 1;

    strncpy(*address, init, addrsize);
    log_wrapper(msgbuffer, srvaddr, end);

    return 0;
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
