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

import com.sun.jna.Structure;

import net.libudi.api.UdiProcessConfig;

/**
 * The process configuration implementation
 *
 * @author mcnulty
 */
public class UdiNativeProcConfig extends Structure {

    /** Needs to be public for JNA purposes */
    public String root_dir;

    /**
     * Constructor.
     *
     * @param source used to create this instance
     */
    public UdiNativeProcConfig(UdiProcessConfig source) {
        this.root_dir = (source == null || source.getRootDir() == null ) ? "" :
                source.getRootDir().toAbsolutePath().toString();
    }

    @Override
    protected List getFieldOrder() {
        return Arrays.asList(new String[] {"root_dir"});
    }
}
