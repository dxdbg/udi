/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import net.libudi.api.exceptions.UdiException;

/**
 * Indicates that the wrapper library encountered an unspecified error
 */
public class NativeLibraryException extends UdiException {

    private static final long serialVersionUID = 7833255232876843889L;

    public NativeLibraryException(String message) {
        super(message);
    }

    public NativeLibraryException(Throwable cause) {
        super(cause);
    }

    public NativeLibraryException(String message, Throwable cause) {
        super(message, cause);
    }
}
