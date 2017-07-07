//
// Created by darkwolf on 02/07/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tp1opt.h"


void check_vality_options(int mode) {
    if (mode) {
        fprintf(stderr, "Invalid options combinations.\n");
        exit(EXIT_FAILURE);
    }
}


void set_options(int argc, char **argv, int is_server, struct netconfigs* netconf) {

    int c, index;
    const char* options;

    // Defaults
    options = (is_server) ? GETOPT_OPTIONS_SERVER : GETOPT_OPTIONS_CLIENT;

    netconf->is_server = is_server;
    netconf->chat_mode_opt = GROUP_MODE_SET;
    netconf->parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
    netconf->transport_protocol_opt = PROT_MODE_UDP;
    netconf->interr_opt = INT_MODE_ENTER;

    netconf->cm_set = netconf->pm_set = netconf->tp_set = netconf->po_set
            = netconf->mc_set = netconf->io_set = 0;

    opterr = 0;

    while ((c = getopt (argc, argv, options)) != -1) {
        switch (c) {
            case OPT_UNIQUE:
                check_vality_options(netconf->cm_set);
                netconf->chat_mode_opt = UNIQUE_MODE_SET;
                printf("%c is used.\n", OPT_UNIQUE);
                netconf->cm_set = 1;
                break;
            case OPT_GROUP:
                check_vality_options(netconf->cm_set);
                netconf->chat_mode_opt = GROUP_MODE_SET;
                printf("%c is used.\n", OPT_GROUP);
                netconf->cm_set = 1;
                break;
            case OPT_FORK:
                check_vality_options(netconf->pm_set);
                netconf->parallelism_mode_opt = MULTIPROCESSING_MODE_SET;
                printf("%c is used.\n", OPT_FORK);
                netconf->pm_set = 1;
                break;
            case OPT_THREAD:
                check_vality_options(netconf->pm_set);
                netconf->parallelism_mode_opt = MULTITHREADING_MODE_SET;
                printf("%c is used.\n", OPT_THREAD);
                netconf->pm_set = 1;
                break;
            case OPT_PROTOCOL_MODE:
                check_vality_options(netconf->tp_set);
                if (strcmp(optarg, PROT_MODE_TCP) == 0
                    || strcmp(optarg, PROT_MODE_UDP) == 0)
                    printf("%s is used.\n", optarg);
                else {
                    fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be used"
                                    " with -%c. Exiting.\n",
                            PROT_MODE_TCP, PROT_MODE_UDP, OPT_PROTOCOL_MODE);
                    exit(EXIT_FAILURE);
                }
                netconf->transport_protocol_opt = optarg;
                netconf->tp_set = 1;
                break;
            case OPT_PORT:
                check_vality_options(netconf->po_set);
                netconf->connection_port = atoi(optarg);
                printf("Port #%d is used.\n", netconf->connection_port);
                netconf->po_set = 1;
                break;
            case OPT_MAX_CONNECTIONS:   // (S.O.)
                if (netconf->is_server) {
                    check_vality_options(netconf->mc_set);
                    int mc = atoi(optarg);
                    if (mc <= 0) {
                        fprintf(stderr, "Max connections must be 1 or bigger. "
                                "Exiting.");
                        exit(EXIT_FAILURE);
                    }
                    netconf->max_connections_opt = mc;
                    printf("Max connections: %d.\n", netconf->max_connections_opt);
                    netconf->mc_set = 1;
                }
                break;
            case OPT_INTERRUPTION:      // (C.O.)
                if (!netconf->is_server) {
                    check_vality_options(netconf->io_set);
                    if (strcmp(optarg, INT_MODE_ENTER) == 0
                        || strcmp(optarg, INT_MODE_INTER) == 0)
                        printf("Interruption: \"%s\" is used.\n", optarg);
                    else {
                        fprintf(stderr, "Invalid option: \"%s\" or \"%s\" must be "
                                        "used with -%c. Exiting.\n",
                                INT_MODE_ENTER, INT_MODE_INTER, OPT_INTERRUPTION);
                        exit(EXIT_FAILURE);
                    }
                    netconf->interr_opt = optarg;
                    netconf->io_set = 1;
                }
                break;
            case '?':
                if (optopt == OPT_PROTOCOL_MODE)
                    fprintf(stderr, "Option -%c requires an argument (%s or %s).",
                            optopt, PROT_MODE_TCP, PROT_MODE_UDP);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.", optopt);
                fprintf(stderr, " Exiting.\n");
                exit(EXIT_FAILURE);
            default:
                exit(EXIT_FAILURE);
        }
    }
    if (!netconf->po_set) {
        fprintf(stderr, "Port number (option -%c) is required. Exiting.\n",
                OPT_PORT);
        exit(EXIT_FAILURE);
    } else if (netconf->is_server && !netconf->mc_set) {
        fprintf(stderr, "Max connections (option -%c) is required for server. "
                        "Exiting.\n", OPT_MAX_CONNECTIONS);
        exit(EXIT_FAILURE);
    }
    for (index = optind; index < argc; index++)
        printf ("Non-option argument: %s\n", argv[index]);

}
