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


void set_options(int argc, char **argv, struct netconfigs* netconf) {

    int c, index;

    // Defaults
    netconf->chat_mode_flag = GROUP_MODE_SET;
    netconf->parallelism_mode_flag = MULTIPROCESSING_MODE;
    netconf->transport_protocol_mode = PROT_MODE_UDP;

    netconf->cm_set = netconf->pm_set = netconf->tp_set = netconf->po_set = 0;

    opterr = 0;

    while ((c = getopt (argc, argv, GETOPT_OPTIONS)) != -1) {
        switch (c) {
            case OPT_UNIQUE:
                check_vality_options(netconf->cm_set);
                netconf->chat_mode_flag = UNIQUE_MODE_SET;
                printf("%c is used.\n", OPT_UNIQUE);
                netconf->cm_set = 1;
                break;
            case OPT_GROUP:
                check_vality_options(netconf->cm_set);
                netconf->chat_mode_flag = GROUP_MODE_SET;
                printf("%c is used.\n", OPT_GROUP);
                netconf->cm_set = 1;
                break;
            case OPT_FORK:
                check_vality_options(netconf->pm_set);
                netconf->parallelism_mode_flag = MULTIPROCESSING_MODE;
                printf("%c is used.\n", OPT_FORK);
                netconf->pm_set = 1;
                break;
            case OPT_THREAD:
                check_vality_options(netconf->pm_set);
                netconf->parallelism_mode_flag = MULTITHREADING_MODE;
                printf("%c is used.\n", OPT_THREAD);
                netconf->pm_set = 1;
                break;
            case OPT_PROTOCOL_MODE:
                check_vality_options(netconf->tp_set);
                if (strcmp(optarg, PROT_MODE_TCP) == 0)
                    printf("%s is used.\n", PROT_MODE_TCP);
                else if (strcmp(optarg, PROT_MODE_UDP) == 0)
                    printf("%s is used.\n", PROT_MODE_UDP);
                else {
                    fprintf(stderr, "Invalid option: %s or %s must be used.\n",
                            PROT_MODE_TCP, PROT_MODE_UDP);
                    exit(EXIT_FAILURE);
                }
                netconf->transport_protocol_mode = optarg;
                netconf->tp_set = 1;
                break;
            case OPT_PORT:
                check_vality_options(netconf->po_set);
                netconf->connection_port = atoi(optarg);
                printf("Port #%d is used.\n", netconf->connection_port);
                netconf->po_set = 1;
                break;
            case '?':
                if (optopt == OPT_PROTOCOL_MODE)
                    fprintf(stderr, "Option -%c requires an argument (%s or %s).\n",
                            optopt, PROT_MODE_TCP, PROT_MODE_UDP);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }
    if (!netconf->po_set) {
        fprintf(stderr, "Port number is requered.\n");
        exit(1);
    }
    for (index = optind; index < argc; index++)
        printf ("Non-option argument: %s\n", argv[index]);

}
