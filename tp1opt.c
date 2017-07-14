//
// Created by darkwolf on 02/07/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include "tp1opt.h"


/* Struct containing infos about options chosen by user at startup
 * */
typedef struct netsettings {
    int is_server;                  // Flag indicating server or client app;
    int chat_mode_opt;              // Chat mode (UNIQUE or GROUP) (S.O.);
    int parallelism_mode_opt;       // Par. mode (MULTIPROCESS or MULTITHREADING);
    int transport_protocol_opt;     // Transport protocl used (TCP or UDP);
    int connection_port;            // Port number used;
    int max_connections_opt;        // Max number of connections accepted (S.O.);
    char* interr_opt;               // Interruption for sending messages (C.O.);
    char* ip_address;               // IPv4 address (C.O.);
} net_opts;


net_opts netconf; // All user settings will be stored here

int netopt_is_set = 0; // Flag indicating if settings are (or not) set


void netopt_debug();

int netopt_is_option_valid(int mode);


int netopt_set(int argc, char **argv, int is_server) {

    int c, qty, index;
    const char* options;
    short cm_set = 0, pm_set = 0, tp_set = 0, cp_set = 0, mc_set = 0, io_set = 0;

    if (netopt_is_set)
        return -1;

    netconf.is_server = (is_server > 0) ? 1 : 0;

    // Defaults
    options = (netconf.is_server) ? GETOPT_OPTIONS_SERVER : GETOPT_OPTIONS_CLIENT;

    netconf.chat_mode_opt = GROUP_MODE_SET;
    netconf.parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
    netconf.transport_protocol_opt = SOCK_STREAM; // TCP
    netconf.interr_opt = INT_MODE_ENTER;

    opterr = 0;

    while ((c = getopt (argc, argv, options)) != -1) {
        switch (c) {
            case OPT_UNIQUE:
                if (!netopt_is_option_valid(cm_set))
                    return -1;
                netconf.chat_mode_opt = UNIQUE_MODE_SET;
                cm_set = 1;
                break;
            case OPT_GROUP:
                if (!netopt_is_option_valid(cm_set))
                    return -1;
                netconf.chat_mode_opt = GROUP_MODE_SET;
                cm_set = 1;
                break;
            case OPT_FORK:
                if (!netopt_is_option_valid(pm_set))
                    return -1;
                netconf.parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
                pm_set = 1;
                break;
            case OPT_THREAD:
                if (!netopt_is_option_valid(pm_set))
                    return -1;
                netconf.parallelism_mode_opt = MULTITHREADING_MODE_SET;
                pm_set = 1;
                break;
            case OPT_PROTOCOL_MODE:
                if (!netopt_is_option_valid(tp_set))
                    return -1;
                if (strcmp(optarg, PROT_MODE_TCP) == 0) {
                    netconf.transport_protocol_opt = SOCK_STREAM;  // TCP
                } else if (strcmp(optarg, PROT_MODE_UDP) == 0) {
                    netconf.transport_protocol_opt = SOCK_DGRAM;   // UDP
                } else {
                    fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be used"
                                    " with -%c.\n",
                            PROT_MODE_TCP, PROT_MODE_UDP, OPT_PROTOCOL_MODE);
                    return -1;
                }
                tp_set = 1;
                break;
            case OPT_PORT:              // (S.O.)
                if (!netopt_is_option_valid(cp_set))
                    return -1;
                netconf.connection_port = atoi(optarg);
                cp_set = 1;
                break;
            case OPT_MAX_CONNECTIONS:   // (S.O.)
                if (!netopt_is_option_valid(mc_set))
                    return -1;
                int mc = atoi(optarg);
                if (mc <= 0) {
                    fprintf(stderr, "Max connections must be 1 or bigger.\n");
                    return -1;
                } else if (mc > 5) {
                    fprintf(stderr, "Max connections must not be too big "
                            "(higher than 5).\n");
                    return -1;
                }
                netconf.max_connections_opt = mc;
                mc_set = 1;
                break;
            case OPT_INTERRUPTION:      // (C.O.)
                if (!netopt_is_option_valid(io_set))
                    return -1;
                if (strcmp(optarg, INT_MODE_ENTER) != 0
                    && strcmp(optarg, INT_MODE_INTER) != 0) {
                    fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be "
                                    "used with -%c.\n",
                            INT_MODE_ENTER, INT_MODE_INTER, OPT_INTERRUPTION);
                    return -1;
                }
                netconf.interr_opt = optarg;
                io_set = 1;
                break;
            case '?':
                if (isprint (optopt))
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                return -1;
            default:
                exit(EXIT_FAILURE);
        }
    }
    if (netconf.is_server) {
        if (!cp_set) {
            fprintf(stderr, "Port number (option '-%c') is required.\n",
                    OPT_PORT);
            return -1;
        } else if (!mc_set) {
            fprintf(stderr, "Max connections (option '-%c') is required for server.\n",
                    OPT_MAX_CONNECTIONS);
            return -1;
        }
    }

    qty = argc - optind;    // Must be two options for client: IP and port
                            // Must be zero for server.
    if (!is_server) {
        if (qty != 2) {
            fprintf(stderr, "Usage: %s [OPTIONS] [IP] [PORT]\n", argv[0]);
            return -1;
        }
        netconf.ip_address = argv[argc - 2];
        netconf.connection_port = atoi(argv[argc - 1]);
    } else {
        if (qty != 0) {
            fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
            for (index = optind; index < argc; index++)
                printf("Unknown argument '%s'\n", argv[index]);
            return -1;
        }
    }
    netopt_debug();
    netopt_is_set = 1;

    return 0;
}


int netopt_is_server() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    return netconf.is_server;
}


int netopt_get_chatmode() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    if (!netconf.is_server)
        return NETOPT_OPTION_NOT_VALID;
    return netconf.chat_mode_opt;
}


int netopt_get_parallelism_mode() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    return netconf.parallelism_mode_opt;
}


int netopt_get_transport_protocol() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    return netconf.transport_protocol_opt;
}


int netopt_get_port() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    return netconf.connection_port;
}


int netopt_get_max_connections_number() {
    if (!netopt_is_set)
        return NETOPT_OPTIONS_NOT_SET;
    if (!netconf.is_server)
        return NETOPT_OPTION_NOT_VALID;
    return netconf.max_connections_opt;
}


char* netopt_get_interruption_key() {
    if (!netopt_is_set || netconf.is_server)
        return "";
    return netconf.interr_opt;
}


char* netopt_get_ip_address() {
    if (netconf.is_server)
        return "";
    return netconf.ip_address;
}


int netopt_is_option_valid(int mode) {
    if (mode) {
        fprintf(stderr, "Invalid options combinations.\n");
        return 0;
    }
    return 1;
}


void netopt_debug() {

    printf("Application started in %s mode.\n",
           (netconf.is_server) ? "Server" : "Client");
    printf("%s mode is used.\n",
           (netconf.chat_mode_opt == UNIQUE_MODE_SET) ? "Unique" : "Group");
    printf("%s mode is used.\n",
            (netconf.parallelism_mode_opt == MULTITHREADING_MODE_SET) ?
            "Multithreading" : "Multiprocessing");
    printf("%s protocol is used.\n",
           (netconf.transport_protocol_opt == SOCK_DGRAM) ? "UDP" : "TCP");
    if (netconf.is_server) {
        printf("Max connections: %d.\n", netconf.max_connections_opt);
    } else {
        printf("Interruption: \"%s\" is used.\n", netconf.interr_opt);
        printf("IP %s is used.\n", netconf.ip_address);
    }
    printf("Port %d is used.\n", netconf.connection_port);

}


void netopt_unset() {
    netopt_is_set = 0;
}
