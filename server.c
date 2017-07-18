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


/* **************************************** */
/* Global, to avoid "value escapes local scope" warning;
 * Common to all threads / forked processes */

fd_set active_sockets;

int rwh_pipe[2];

/* **************************************** */


/* **************************************** */
/* Structs used */

// Reader-writer handlers packet: Contain info communicated between handlers
struct rwh_packet {
    int sockfd;
    struct sockaddr_in client;
    int clilen;
    char log[LOG_BUFFER_SIZE];
};

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
    int i, optval = 1, n;
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

    // Remove the time to wait after closing server
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void*) &optval, sizeof(int));

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
    n = netopt_get_transport_protocol() == SOCK_STREAM;
    slist_start((size_t) netopt_get_max_connections_number(), n);

    // Initialize pipe communication between reader and writer
    if (pipe(rwh_pipe) < 0) {
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
            close(rwh_pipe[1]); // Writer handler will not write nothing
            writer_handler((void*) server_address);
            return EXIT_SUCCESS;
        } else {
            // TODO use shared memory to grant access (in child process) to slist
            close(rwh_pipe[0]); // Reader handler will not read nothing
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
                reader_handler((void *) &i);
            }
        }
    }
    close(sockfd);
    slist_finalize();   // Free allocated memory to slist
    netopt_unset();     // Free allocated memory to netopt

    puts("Bye!");

    return EXIT_SUCCESS;
}


void accept_connections_handler(int listen_socket) {

    struct rwh_packet s;
    char address[SLIST_ADDR_MAX_SIZE];
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    if (netopt_get_transport_protocol() == SOCK_DGRAM)
        return;

    int newsockfd = accept(listen_socket, (struct sockaddr*) &cli_addr,
                       (unsigned int*) &clilen);
    if (newsockfd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Fill data to insert in connection's list
    addr_wrapper(&address, cli_addr);

    // Insert into list
    if (slist_push(newsockfd, cli_addr) == SLIST_MAX_SIZE_REACHED) {
        send_unit_message(newsockfd,
                          "Max connections reached. Connection refused.\n");
        return;
    }

    FD_SET(newsockfd, &active_sockets);

    // Fill log_buffer to format message
    memset((char*) &s, 0, sizeof(s));
    s.sockfd = NULL_SOCKET;
    printf(client_logon_message(&s.log, slist_get_address_by_socket(newsockfd)));

    if (netopt_get_chatmode() == GROUP_MODE) // Send message to writer
        write(rwh_pipe[1], (char*) &s, sizeof(s));

}


void* writer_handler(void *arg) {

    struct rwh_packet s;
    char addr[SLIST_ADDR_MAX_SIZE];
    char *retvalue;
    char *server_address = (char*) arg;
    conn_t* c;

    while (!is_exit) {

        memset(&s.log, 0, sizeof(s.log));

        if (netopt_get_chatmode() == UNIQUE_MODE) {
            do {
                retvalue = fgets(s.log, sizeof(s.log), stdin);
            } while (retvalue == NULL);
            if (locate_addressee(server_address, &s.log, &addr) < 0) {
                puts("Message not send due to incorrect format.");
                continue;
            }
            c = slist_get_by_address(addr);
            if (c == NULL) {
                puts("Client not found or not connected (error on get socket).");
                continue;
            }
            ssize_t wt = sendto(c->sockfd, s.log, sizeof(s.log), 0,
                                (struct sockaddr *) &c->info,
                                (socklen_t) c->infolen);
            if (wt < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            } else if (wt == 0) {
                puts("Client not found or not connected (error on write).");
                continue;
            }
            printf(s.log);

        } else { // Receive message from reader

            // Pass along the message to clients
            if (read(rwh_pipe[0], (char*) &s, sizeof(s)) > 0)
                slist_sendall(s.log, s.sockfd, s.client);
        }
    }
    return NULL;
}


void* reader_handler(void *arg) {

    struct rwh_packet s;
    int n;

    memset(&s.log, 0, sizeof(s.log));
    s.sockfd = *((int*) arg);

    ssize_t rd = recvfrom(s.sockfd, s.log, sizeof(s.log), 0,
                          (struct sockaddr *) &s.client, (socklen_t *) &s.clilen);
    if (rd < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    } else if (rd == 0) { // Client has logged out
        if (netopt_get_transport_protocol() == SOCK_STREAM) { // TCP
            n = slist_pop(s.sockfd); // Close occurs here
            assert(n == SLIST_OK);
            FD_CLR(s.sockfd, &active_sockets);
            client_logout_message(&s.log, slist_get_address_by_socket(s.sockfd));
        }
    }
    if (netopt_get_transport_protocol() == SOCK_DGRAM) {
        printf("Received packet from: %s:%hu\n",
               inet_ntoa(s.client.sin_addr), ntohs(s.client.sin_port));
        slist_push(s.sockfd, s.client);
    }
    printf(s.log);
    if (netopt_get_chatmode() == GROUP_MODE) {
        write(rwh_pipe[1], (char*) &s, sizeof(s));
    }

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
    char localbuffer[LOG_BUFFER_SIZE];

    if (strstr(*msgbuffer, SEND_MSG_CMD) != *msgbuffer)
        return -1;

    memset(localbuffer, 0, sizeof(localbuffer));
    memset(address, 0, sizeof(*address));
    strcpy(localbuffer, *msgbuffer);

    init = strchr(localbuffer, CMD_DELIMITER);
    if (init == NULL)
        return -1;
    end = strchr(++init, CMD_DELIMITER);
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
