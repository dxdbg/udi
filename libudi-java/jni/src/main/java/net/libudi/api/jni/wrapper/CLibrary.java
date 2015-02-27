
/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package net.libudi.api.jni.wrapper;

import com.sun.jna.Library;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;

/**
 * Provides access to libc for use with libudi
 *
 * @author mcnulty
 */
public interface CLibrary extends Library {

    /** Library name for libc */
    public static final String LIBRARY_NAME = (Platform.isWindows()) ? "msvcrt" : "c";

    /**
     * @param ptr the pointer returned a libudi function
     */
    void free(Pointer ptr);
}
