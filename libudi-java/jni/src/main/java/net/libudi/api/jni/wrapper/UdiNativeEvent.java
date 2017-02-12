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
import com.sun.jna.Structure.ByReference;

/**
 * Structure for a native udi_event
 */
public class UdiNativeEvent extends Structure {

    public static class ByReference extends UdiNativeEvent implements Structure.ByReference {}

    public UdiNativeEvent()
    {
    }

    public UdiNativeEvent(Pointer p)
    {
        super(p);
        this.read();
    }

    public int event_type;

    public Pointer proc;

    public Pointer thr;

    public Pointer event_data;

    public ByReference next_event;

    @Override
    protected List getFieldOrder() {
        return Arrays.asList(new String[]{
                "event_type",
                "proc",
                "thr",
                "event_data",
                "next_event"
        });
    }
}
