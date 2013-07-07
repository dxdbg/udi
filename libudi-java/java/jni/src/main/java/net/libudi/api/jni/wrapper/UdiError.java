/*
 * Copyright (c) 2011-2013, Dan McNulty
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the UDI project nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
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
