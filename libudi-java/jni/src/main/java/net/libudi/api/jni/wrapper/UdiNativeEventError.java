/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.wrapper;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Represents the event_data for an event with type ERROR
 *
 * @author mcnulty
 */
public class UdiNativeEventError extends Structure {

    public String errstr;

    public UdiNativeEventError(Pointer p) {
        super(p);
        read();
    }

    @Override
    protected List getFieldOrder() {
        return Arrays.asList(new String[] {"errstr"});
    }
}
