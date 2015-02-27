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
 *
 * @author mcnulty
 */
public enum UdiError {

    /** indicates an internal library error */
    LIBRARY(0),

    /** indicates there was an error processing the request */
    REQUEST(1),

    /** there was no error */
    NONE(2);

    private final int errorCode;

    /**
     * Constructor.
     *
     * @param errorCode the error codde
     */
    private UdiError(int errorCode) {
        this.errorCode = errorCode;
    }

    /**
     * @param errorCode the error code
     * @return the corresponding error
     */
    public static UdiError fromErrorCode(int errorCode) {
        switch (errorCode) {
            case 0:
                return LIBRARY;
            case 1:
                return REQUEST;
            case 2:
                return NONE;
            default:
                throw new IllegalArgumentException("Unknown error code: " + errorCode);
        }
    }

    /**
     * @param errorCode the error code
     * @param errMsg the error message
     *
     * @return the exception encapsulating the error message (or null for no exception)
     */
    public static UdiException toException(int errorCode, String errMsg) {
        return toException(fromErrorCode(errorCode), errMsg);
    }

    /**
     * @param error the error
     * @param errMsg the error message
     *
     * @return the exception encapsulating the error message (or null for no exception)
     */
    public static UdiException toException(UdiError error, String errMsg) {
        switch (error) {
            case LIBRARY:
                return new InternalLibraryException(errMsg);
            case REQUEST:
                return new RequestException(errMsg);
            case NONE:
                return null;
            default:
                throw new IllegalArgumentException("Unknown error: " + error);
        }
    }
}
