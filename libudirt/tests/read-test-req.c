/*
 * Copyright (c) 2011-2017, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt-malloc.c"
#include "udirt-msg.c"

#include "test-lib.h"
#include "mock-lib.h"

int testing_udirt() {
    return 1;
}

struct test_req {
    uint32_t field1;
    const char *field2;
};

static
void field1_callback(void *ctx, uint32_t value) {
    struct test_req *req = (struct test_req *)req_state(ctx)->data;
    req->field1 = value;

    complete_item(ctx);
}

static
void field2_callback(void *ctx, cbor_data data, size_t len) {
    struct test_req *req = (struct test_req *)req_state(ctx)->data;
    char *field2 = (char *)udi_malloc(len+1);
    if (field2 == NULL) {
        abort();
    }
    memcpy(field2, data, len);
    field2[len] = '\0';

    req->field2 = field2;

    complete_item(ctx);
}

int main(int argc, char *argv[]) {
    struct msg_item fields[2];
    struct msg_config config;

    config.items = fields;
    config.num_items = 2;

    fields[0].key = "field1";
    fields[0].callbacks = invalid_callbacks;
    fields[0].callbacks.uint32 = field1_callback;

    fields[1].key = "field2";
    fields[1].callbacks = invalid_callbacks;
    fields[1].callbacks.string = field2_callback;

    init_req_handling();

    struct test_req req;
    udi_errmsg errmsg;

    memset(&req, 0, sizeof(req));
    memset(&errmsg, 0, sizeof(errmsg));
    errmsg.size = ERRMSG_SIZE;

    cbor_item_t *root = cbor_new_definite_map(2);

    struct cbor_pair field1_pair;
    field1_pair.key = cbor_move(cbor_build_string("field1"));
    field1_pair.value = cbor_move(cbor_build_uint32(13));
    cbor_map_add(root, field1_pair);

    struct cbor_pair field2_pair;
    field2_pair.key = cbor_move(cbor_build_string("field2"));
    field2_pair.value = cbor_move(cbor_build_string("test string"));
    cbor_map_add(root, field2_pair);

    cbor_mutable_data buffer = NULL;
    size_t buffer_size = 0;
    size_t length = cbor_serialize_alloc(root, &buffer, &buffer_size);
    test_assert(length != 0);
    test_assert(buffer != NULL);
    test_assert(buffer_size == length);

    add_read_data(buffer, length);
    udi_free(buffer);
    cbor_decref(&root);

    int result = read_request_data(1, &config, &req, &errmsg);
    test_assert(RESULT_SUCCESS == result);
    test_assert(req.field1 == 13);
    test_assert(strcmp(req.field2, "test string") == 0);

    cleanup_mock_lib();

    return EXIT_SUCCESS;
}
