/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * Header for test test library
 *
 * @file test-lib.h
 */
#ifndef _TEST_LIB_H
#define _TEST_LIB_H 1

#include <stdio.h>
#include <stdlib.h>

#define test_assert(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "assertion failed at [%s:%d]\n", __FILE__, __LINE__); \
            abort(); \
        } \
    }while(0)

#define test_assert_msg(msg, condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "assertion failed at [%s:%d]: %s\n", __FILE__, __LINE__, msg); \
            abort(); \
        } \
    }while(0)

#endif /* _TEST_LIB_H */

