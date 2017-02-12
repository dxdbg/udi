/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.ut;

import java.io.IOException;

import net.libudi.api.UdiProcessManager;
import net.libudi.api.jni.impl.UdiProcessManagerImpl;

/**
 * Implementation of unit tests for native implementation of the libudi api
 */
public class NativeApiTest extends BaseApiUt {

    private final UdiProcessManagerImpl impl = new UdiProcessManagerImpl();

    /**
     * Constructor.
     *
     * @throws IOException on failure to read the native file tests info
     */
    public NativeApiTest() throws IOException
    {
    }

    @Override
    protected UdiProcessManager getProcessManager() {
        return impl;
    }
}
