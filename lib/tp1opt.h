//
// Created by darkwolf on 02/07/17.
//

#ifndef TP1_TP1OPT_H
#define TP1_TP1OPT_H

#include <unistd.h>

// Options (getopt)

#define GETOPT_OPTIONS_SERVER "ugp:m:l:d"
#define GETOPT_OPTIONS_CLIENT "ftm:s:d"

#define OPT_UNIQUE 'u'              // Unique mode (SERVER ONLY);
#define OPT_GROUP 'g'               // Group mode (default) (SERVER ONLY);

#define OPT_FORK 'f'                // Fork process mode (default) (CLIENT ONLY);
#define OPT_THREAD 't'              // Thread mode (CLIENT ONLY);

#define OPT_PROTOCOL_MODE 'm'       // Transport protocol used, which can be:
#define PROT_MODE_UDP "udp"             // UDP;
#define PROT_MODE_TCP "tcp"             // TCP;

#define OPT_PORT 'p'                // Port used (required) (SERVER ONLY);

#define OPT_MAX_CONNECTIONS 'l'     // Max qty. connections (required) (SERVER ONLY);

#define OPT_INTERRUPTION 's'        // Set interruption for send messages
                                    // (CLIENT ONLY), which can be:
#define INT_MODE_ENTER "enter"          // Enter key;
#define INT_MODE_INTER "inter"          // Interuption key defined;

#define OPT_DEBUG 'd'               // Start in debug mode

#define UNIQUE_MODE 0
#define GROUP_MODE 1                // Default
#define FORK_MODE 0                 // Default
#define MULTITHREADING_MODE 1

#define NETOPT_OPTION_NOT_VALID -1
#define NETOPT_OPTIONS_NOT_SET -2


int netopt_set(int argc, char **argv, int is_server);

int netopt_is_server();

int netopt_is_debug_mode();

int netopt_get_chatmode();

int netopt_get_multiprocessing_mode();

int netopt_get_transport_protocol();

int netopt_get_port();

int netopt_get_max_connections_number();

char* netopt_get_interruption_key();

char* netopt_get_ip_address();

void netopt_unset();


#endif //TP1_TP1OPT_H
