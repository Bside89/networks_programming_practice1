#include <stdlib.h>
#include "tp1opt.h"


int main(int argc, char *argv[]) {

    struct netconfigs options;

    set_options(argc, argv, 1, &options);

    return EXIT_SUCCESS;
}