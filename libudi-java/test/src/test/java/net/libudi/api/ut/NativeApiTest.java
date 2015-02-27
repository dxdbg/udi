/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.ut;

import net.libudi.api.UdiProcessManager;
import net.libudi.api.jni.impl.UdiProcessManagerImpl;

/**
 * Implementation of unit tests for Native implementation of libudi api
 *
 * @author mcnulty
 */
public class NativeApiTest extends BaseApiUt {

    private final UdiProcessManagerImpl impl = new UdiProcessManagerImpl();

    @Override
    protected UdiProcessManager getProcessManager() {
        return impl;
    }
}
