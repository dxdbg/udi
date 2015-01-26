/*
 * Copyright (c) 2011-2015, Dan McNulty
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "libuditest.h"

/**
 * Entry point for running tests
 */
int main(int argc, char *argv[]) {
    return UDITestCase::executeTests(argc, argv);
}
