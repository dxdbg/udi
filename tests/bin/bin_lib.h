/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _BIN_LIB_H_
#define _BIN_LIB_H_ 1

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int bin_debug_enable;

void init_bin();

long udi_get_thread_id(int pid);

#define bin_printf(format, ...) \
    do {\
        if ( bin_debug_enable ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                ## __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
} // extern C
#endif

#endif
