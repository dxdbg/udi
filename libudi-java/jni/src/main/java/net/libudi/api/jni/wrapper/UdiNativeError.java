/*
 * Copyright (c) 2011-2017, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.wrapper;

import java.util.Arrays;
import java.util.List;
import java.util.Optional;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Structure.ByValue;

import net.libudi.api.exceptions.UdiException;

/**
 * Structure for native udi_error struct
 */
public class UdiNativeError extends Structure implements ByValue, AutoCloseable
{
    public int code;

    public Pointer msg;

    private String errorMessage;

    public Optional<String> getErrorMessage()
    {
        if (errorMessage == null && msg != Pointer.NULL) {
            errorMessage = msg.getString(0);
        }

        return Optional.ofNullable(errorMessage);
    }

    @Override
    protected List getFieldOrder()
    {
        return Arrays.asList(new String[] {
                "code",
                "msg"
        });
    }

    @Override
    public void close()
    {
        NativeLibraryHandles.INSTANCE.getCLibrary().free(msg);
    }

    public void checkException() throws UdiException
    {
        UdiError.toException(code, getErrorMessage().orElse("no message specified"));
    }
}
