/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt.c"

#include "test-lib.h"
#include "mock-lib.h"

int testing_udirt() {
    return 1;
}

static
void test_literal() {
    char buf[50];

    memset(buf, 0, 50);

    udi_log_formatted("literal message", "source.c", 100);

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("source.c[100]: literal message\n", buf) == 0);
    reset_mock_data();
}

static
void test_integer() {
    char buf[50];

    memset(buf, 0, 50);
    char *file = "source.c";
    int line = 100;
    udi_log_formatted("%d", file, line, -123);

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("source.c[100]: -123\n", buf) == 0);
    reset_mock_data();
}

static
void test_error() {
    char buf[50];
    memset(buf, 0, 50);

    udi_log_formatted_noprefix("%e", EINVAL);

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("Invalid argument", buf) == 0);
    reset_mock_data();
}

static
void test_address() {
    char buf[50];
    memset(buf, 0, 50);

    udi_log_formatted_noprefix("%a", 0x7fff7b218000);

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("0x00007fff7b218000", buf) == 0);
    reset_mock_data();
}

static
void test_string() {
    char buf[50];
    memset(buf, 0, 50);

    udi_log_formatted_noprefix("%s", "Error message");

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("Error message", buf) == 0);
    reset_mock_data();
}

static
void test_byte() {
    char buf[50];
    memset(buf, 0, 50);

    udi_log_formatted_noprefix("%b", 10);

    mock_data_to_buffer(get_written_data(), buf, 50);
    test_assert_msg(buf, strcmp("0a", buf) == 0);
    reset_mock_data();
}

static
void test_formatted_str() {
    char buf[17];
    memset(buf, 0, 17);
    udi_formatted_str(buf, 17, "%x", 0x7fff7b218000);
    test_assert_msg(buf, strcmp("00007fff7b218000", buf) == 0);

    char buf2[16];
    memset(buf2, 0, 16);
    udi_formatted_str(buf2, 16, "%x", 0x7fff7b218000);
    test_assert_msg(buf2, strcmp("00007fff7b21800", buf2) == 0);

    char buf3[32];
    memset(buf3, 'z', 32);
    udi_formatted_str(buf3, 32, "%x", 0x7fff7b218000);
    test_assert_msg(buf3, strcmp("00007fff7b218000", buf3) == 0);
}

int main(int argc, char *argv[]) {

    udi_debug_on = 1;

    init_thread_support();

    test_literal();
    test_integer();
    test_error();
    test_address();
    test_string();
    test_byte();
    test_formatted_str();

    cleanup_mock_lib();

    return EXIT_SUCCESS;
}
