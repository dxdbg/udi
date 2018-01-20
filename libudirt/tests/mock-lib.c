/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>
#include <string.h>

#include "udirt-platform.h"

#include "mock-lib.h"

static struct mock_data *write_data;
static uint8_t *read_data = NULL;
static uint8_t read_data_idx = 0;
static uint8_t read_data_size = 0;

static
void save_data(struct mock_data **global, const uint8_t *data, size_t len) {
    struct mock_data *saved = (struct mock_data *)malloc(sizeof(struct mock_data));
    if (saved == NULL) {
        abort();
    }
    saved->len = len;
    saved->next = NULL;
    saved->data = (uint8_t *)malloc(saved->len);
    if (saved->data == NULL) {
        abort();
    }
    memcpy(saved->data, data, saved->len);

    if (*global == NULL) {
        *global = saved;
    }else{
        struct mock_data *end = *global;
        while (end->next != NULL) {
            end = end->next;
        }

        end->next = saved;
    }
}

void add_read_data(const uint8_t *data, size_t len) {
    read_data = (uint8_t *)realloc(read_data, read_data_size + len);
    if (read_data == NULL) {
        abort();
    }
    memcpy(read_data + read_data_size, data, len);
    read_data_size += len;
}

const struct mock_data *get_written_data() {
    return write_data;
}

void mock_data_to_buffer(const struct mock_data *data, char *buf, size_t len) {

    const struct mock_data *iter = data;

    size_t idx = 0;
    while (iter != NULL && idx < len) {
        size_t copy_len;
        if (idx + iter->len > len-1) {
            copy_len = ((len-1) - idx) + 1;
        } else {
            copy_len = iter->len;
        }

        memcpy(buf + idx, iter->data, copy_len);

        idx += copy_len;
        iter = iter->next;
    }
}

static
void free_mock_data(struct mock_data *data) {

    struct mock_data *iter = data;
    while (iter != NULL) {
        struct mock_data *cur = iter;
        iter = iter->next;
        free(cur->data);
        free(cur);
    }
}

void reset_mock_data() {
    free_mock_data(write_data);
    write_data = NULL;
}

void cleanup_mock_lib() {
    free(read_data);
    free_mock_data(write_data);
}

int read_from(udirt_fd fd, uint8_t *dst, size_t length) {
    if (read_data == NULL) {
        abort();
    }

    if (length > (read_data_size - read_data_idx)) {
        abort();
    }

    memcpy(dst, read_data + read_data_idx, length);
    read_data_idx += length;

    return 0;
}

int write_to(udirt_fd fd, const uint8_t *src, size_t length) {
    save_data(&write_data, src, length);
    return 0;
}

udirt_fd udi_log_fd() {
    return -1;
}

