#include <stdlib.h>

#include "bin_lib.h"

static const char *DEBUG_ENV = "UDI_TEST_DEBUG";

int bin_debug_enable = 0;

void init_bin() {
    if ( getenv(DEBUG_ENV) != NULL ) {
        bin_debug_enable = 1;
    }
}
