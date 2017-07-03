//
// Created by darkwolf on 02/07/17.
//

#ifndef TP1_TP1OPT_H
#define TP1_TP1OPT_H

#include <unistd.h>

// Options (getopt)

#define GETOPT_OPTIONS "ugftp:m:"

#define OPT_UNIQUE 'u'          // Unique mode
#define OPT_GROUP 'g'           // Group mode (default)

#define OPT_FORK 'f'            // Fork process mode (default)
#define OPT_THREAD 't'          // Thread mode

#define OPT_PROTOCOL_MODE 'm'   // Transport protocol used, which can be:
#define PROT_MODE_UDP "udp"         // UDP
#define PROT_MODE_TCP "tcp"         // TCP

#define OPT_PORT 'p'            // Port used (required)

#define UNIQUE_MODE_SET 0
#define GROUP_MODE_SET 1        // Default
#define MULTIPROCESSING_MODE 0  // Default
#define MULTITHREADING_MODE 1


struct netconfigs {
    int chat_mode_opt;
    int cm_set;
    int parallelism_mode_opt;
    int pm_set;
    char* transport_protocol_opt;
    int tp_set;
    int connection_port;
    int po_set;
};


void set_options(int argc, char **argv, struct netconfigs* netconf);

void check_vality_options(int mode);


#endif //TP1_TP1OPT_H
