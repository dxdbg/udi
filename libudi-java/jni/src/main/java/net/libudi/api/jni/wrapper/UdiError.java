/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.wrapper;

import net.libudi.api.exceptions.InternalLibraryException;
import net.libudi.api.exceptions.RequestException;
import net.libudi.api.exceptions.UdiException;

/**
 * Encapsulation for an error returned by a libudi exported function
 */
public enum UdiError {

    /** indicates an internal library error */
    LIBRARY(0),

    /** indicates there was an error processing the request */
    REQUEST(1),

    /** indicates a memory allocation error was encountered */
    NO_MEM(2),

    /** there was no error */
    NONE(3);

    private final int errorCode;

    /**
     * Constructor.
     *
     * @param errorCode the error codde
     */
    UdiError(int errorCode) {
        this.errorCode = errorCode;
    }

    /**
     * @param errorCode the error code
     * @return the corresponding error
     */
    public static UdiError fromErrorCode(int errorCode) {

        for (UdiError error : UdiError.values()) {
            if (error.errorCode == errorCode) {
                return error;
            }
        }

        throw new IllegalArgumentException("Unknown error code: " + errorCode);
    }

    /**
     * @param errorCode the error code
     * @param errMsg the error message
     *
     * @throws UdiException when the specified error code does not correspond to the NONE error code
     */
    public static void toException(int errorCode, String errMsg) throws UdiException {
        toException(fromErrorCode(errorCode), errMsg);
    }

    /**
     * @param error the error
     * @param errMsg the error message
     *
     * @throws UdiException when the specified error code does not correspond to the NONE error code
     */
    public static void toException(UdiError error, String errMsg) throws UdiException {
        switch (error) {
            case LIBRARY:
                throw new InternalLibraryException(errMsg);
            case REQUEST:
                throw new RequestException(errMsg);
            case NO_MEM:
                throw new OutOfMemoryError();
            case NONE:
                return;
            default:
                throw new IllegalArgumentException("Unknown error: " + error);
        }
    }
}
