#include <stdio.h>
#include <stdlib.h>
#include "tp1opt.h"


int main(int argc, char** argv) {

    struct netconfigs options;              // Struct for store program's configs

    // Get all configs by user
    if (set_options(argc, argv, 1, &options) < 0) {
        fprintf(stderr, "Exiting.\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
