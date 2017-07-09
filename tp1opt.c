//
// Created by darkwolf on 02/07/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include "tp1opt.h"


void debug_options(int is_server, struct netconfigs* netconf);


int is_option_valid(int mode);


int set_options(int argc, char **argv, int is_server, struct netconfigs* netconf) {

    int c, qty, index;
    const char* options;
    short cm_set = 0, pm_set = 0, tp_set = 0, cp_set = 0, mc_set = 0, io_set = 0;

    netconf->is_server = is_server;

    // Defaults
    options = (is_server) ? GETOPT_OPTIONS_SERVER : GETOPT_OPTIONS_CLIENT;

    netconf->chat_mode_opt = GROUP_MODE_SET;
    netconf->parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
    netconf->transport_protocol_opt = SOCK_STREAM; // TCP
    netconf->interr_opt = INT_MODE_ENTER;

    opterr = 0;

    while ((c = getopt (argc, argv, options)) != -1) {
        switch (c) {
            case OPT_UNIQUE:
                if (!is_option_valid(cm_set))
                    return -1;
                netconf->chat_mode_opt = UNIQUE_MODE_SET;
                cm_set = 1;
                break;
            case OPT_GROUP:
                if (!is_option_valid(cm_set))
                    return -1;
                netconf->chat_mode_opt = GROUP_MODE_SET;
                cm_set = 1;
                break;
            case OPT_FORK:
                if (!is_option_valid(pm_set))
                    return -1;
                netconf->parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
                pm_set = 1;
                break;
            case OPT_THREAD:
                if (!is_option_valid(pm_set))
                    return -1;
                netconf->parallelism_mode_opt = MULTITHREADING_MODE_SET;
                pm_set = 1;
                break;
            case OPT_PROTOCOL_MODE:
                if (!is_option_valid(tp_set))
                    return -1;
                if (strcmp(optarg, PROT_MODE_TCP) == 0) {
                    netconf->transport_protocol_opt = SOCK_STREAM;  // TCP
                } else if (strcmp(optarg, PROT_MODE_UDP) == 0) {
                    netconf->transport_protocol_opt = SOCK_DGRAM;   // UDP
                } else {
                    fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be used"
                                    " with -%c.\n",
                            PROT_MODE_TCP, PROT_MODE_UDP, OPT_PROTOCOL_MODE);
                    return -1;
                }
                tp_set = 1;
                break;
            case OPT_PORT:              // (S.O.)
                if (!is_option_valid(cp_set))
                    return -1;
                netconf->connection_port = atoi(optarg);
                cp_set = 1;
                break;
            case OPT_MAX_CONNECTIONS:   // (S.O.)
                if (!is_option_valid(mc_set))
                    return -1;
                int mc = atoi(optarg);
                if (mc <= 0) {
                    fprintf(stderr, "Max connections must be 1 or bigger.\n");
                    return -1;
                }
                netconf->max_connections_opt = mc;
                mc_set = 1;
                break;
            case OPT_INTERRUPTION:      // (C.O.)
                if (!is_option_valid(io_set))
                    return -1;
                if (strcmp(optarg, INT_MODE_ENTER) != 0
                    && strcmp(optarg, INT_MODE_INTER) != 0) {
                    fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be "
                                    "used with -%c.\n",
                            INT_MODE_ENTER, INT_MODE_INTER, OPT_INTERRUPTION);
                    return -1;
                }
                netconf->interr_opt = optarg;
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
    if (netconf->is_server) {
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
        netconf->ip_address = argv[argc - 2];
        netconf->connection_port = atoi(argv[argc - 1]);
    } else {
        if (qty != 0) {
            fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
            for (index = optind; index < argc; index++)
                printf("Unknown argument '%s'\n", argv[index]);
            return -1;
        }
    }
    debug_options(is_server, netconf);

    return 0;
}


int is_option_valid(int mode) {
    if (mode) {
        fprintf(stderr, "Invalid options combinations.\n");
        return 0;
    }
    return 1;
}


void debug_options(int is_server, struct netconfigs* netconf) {

    printf("Application started in %s mode.\n", (is_server) ? "Server" : "Client");
    printf("%s mode is used.\n",
           (netconf->chat_mode_opt == UNIQUE_MODE_SET) ? "Unique" : "Group");
    printf("%s mode is used.\n",
            (netconf->parallelism_mode_opt == MULTITHREADING_MODE_SET) ?
            "Multithreading" : "Multiprocessing");
    printf("%s protocol is used.\n",
           (netconf->transport_protocol_opt == SOCK_DGRAM) ? "UDP" : "TCP");
    if (is_server) {
        printf("Max connections: %d.\n", netconf->max_connections_opt);
    } else {
        printf("Interruption: \"%s\" is used.\n", netconf->interr_opt);
        printf("IP %s is used.\n", netconf->ip_address);
    }
    printf("Port %d is used.\n", netconf->connection_port);

}