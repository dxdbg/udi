/*
 * Copyright (c) 2011-2017, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.wrapper;

import java.util.HashMap;
import java.util.Map;

import com.sun.jna.Library;
import com.sun.jna.Native;

/**
 * A singleton that holds references to the native library
 */
public enum NativeLibraryHandles
{
    INSTANCE;

    private final UdiLibrary udiLibrary;
    private final CLibrary cLibrary;

    NativeLibraryHandles()
    {
        String libraryPath = System.getProperty("udi.native.lib.searchPath");
        if (libraryPath != null) {
            System.setProperty("jna.library.path", libraryPath);
        }

        Map<String, Boolean> options = new HashMap<>();
        options.put(Library.OPTION_ALLOW_OBJECTS, Boolean.TRUE);
        udiLibrary = Native.loadLibrary(UdiLibrary.LIBRARY_NAME, UdiLibrary.class, options);

        cLibrary = Native.loadLibrary(CLibrary.LIBRARY_NAME, CLibrary.class);
    }

    public UdiLibrary getUdiLibrary()
    {
        return udiLibrary;
    }

    public CLibrary getCLibrary()
    {
        return cLibrary;
    }
}
