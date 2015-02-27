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
 * Structure for event_data for event with type THREAD_CREATE
 *
 * @author mcnulty
 */
public class UdiNativeEventThreadCreate extends Structure {

    public Pointer new_thr;

    public UdiNativeEventThreadCreate(Pointer p) {
        super(p);
        read();
    }

    @Override
    protected List getFieldOrder() {
        return Arrays.asList(new String[] {"new_thr"});
    }
}
