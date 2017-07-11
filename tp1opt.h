//
// Created by darkwolf on 02/07/17.
//

#ifndef TP1_TP1OPT_H
#define TP1_TP1OPT_H

#include <unistd.h>

// Options (getopt)

#define GETOPT_OPTIONS_SERVER "ugftp:m:l:"
#define GETOPT_OPTIONS_CLIENT "ugftm:s:"

#define OPT_UNIQUE 'u'              // Unique mode;
#define OPT_GROUP 'g'               // Group mode (default);

#define OPT_FORK 'f'                // Fork process mode (default);
#define OPT_THREAD 't'              // Thread mode;

#define OPT_PROTOCOL_MODE 'm'       // Transport protocol used, which can be:
#define PROT_MODE_UDP "udp"             // UDP;
#define PROT_MODE_TCP "tcp"             // TCP;

#define OPT_PORT 'p'                // Port used (required) (SERVER ONLY);

#define OPT_MAX_CONNECTIONS 'l'     // Max qty. connections (required) (SERVER ONLY);

#define OPT_INTERRUPTION 's'        // Set interruption for send messages
                                    // (CLIENT ONLY), which can be:
#define INT_MODE_ENTER "enter"          // Enter key;
#define INT_MODE_INTER "inter"          // Interuption key defined;

#define UNIQUE_MODE_SET 0
#define GROUP_MODE_SET 1            // Default
#define MULTIPROCESSING_MODE_SET 0  // Default
#define MULTITHREADING_MODE_SET 1


/* Struct containing infos about options chosen by user at startup
 * */
struct netsettings {
    int is_server;                  // Flag indicating server or client app;
    int chat_mode_opt;              // Chat mode (UNIQUE or GROUP);
    int parallelism_mode_opt;       // Par. mode (MULTIPROCESS or MULTITHREADING);
    int transport_protocol_opt;     // Transport protocl used (TCP or UDP);
    int connection_port;            // Port number used;
    int max_connections_opt;        // Max number of connections accepted (S.O.);
    char* interr_opt;               // Interruption for sending messages (C.O.);
    char* ip_address;               // IPv4 address (C.O.);
};


int set_options(int argc, char **argv, int is_server, struct netsettings* netconf);

void sigint_handler(int signum);

void sigterm_handler(int signum);


#endif //TP1_TP1OPT_H
