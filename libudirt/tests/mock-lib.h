/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * Header for test mock library
 *
 * @file mock-lib.h
 */
#ifndef _MOCK_LIB_H
#define _MOCK_LIB_H 1

#include <stdlib.h>
#include <stdint.h>

struct mock_data {
    uint8_t *data;
    size_t len;
    struct mock_data *next;
};

void add_read_data(const uint8_t *data, size_t len);
const struct mock_data *get_written_data();
void mock_data_to_buffer(const struct mock_data *data, char *buf, size_t len);
void reset_mock_data();
void cleanup_mock_lib();

#endif
