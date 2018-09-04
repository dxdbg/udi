/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt.h"
#include "udirt-x86.h"

uint64_t get_register_ud_type(ud_type_t reg, const void *context) {

    USE(reg);
    USE(context);

    return 0;
}

uint64_t get_flags(const void *context) {
    USE(context);

    return 0;
}
