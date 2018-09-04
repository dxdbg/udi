/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt.h"

#include "windows.h"

udirt_fd udi_log_fd() {

    return INVALID_HANDLE_VALUE;
}

const udirt_fd TEST_FD = INVALID_HANDLE_VALUE;
