/*
 * Copyright (c) 2011-2015, Dan McNulty
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.exceptions;

/**
 * Indicates the libudi library had an internal error
 *
 * @author mcnulty
 */
public class InternalLibraryException extends UdiException {

    /** autogenerated serialVersionUID */
    private static final long serialVersionUID = -8386830249839199735L;

    /**
     * Constructor.
     *
     * @param message the detailed message
     */
    public InternalLibraryException(String message) {
        super(message);
    }

    /**
     * Constructor.
     *
     * @param cause the cause
     */
    public InternalLibraryException(Throwable cause) {
        super(cause);
    }

    /**
     * Constructor.
     *
     * @param message the detailed message
     * @param cause the cause
     */
    public InternalLibraryException(String message, Throwable cause) {
        super(message, cause);
    }
}
