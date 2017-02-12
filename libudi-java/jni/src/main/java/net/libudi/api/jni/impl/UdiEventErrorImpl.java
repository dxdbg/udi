/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEventError;
import net.libudi.api.event.UdiEventVisitor;

/**
 * Implementation of UdiEventError
 */
public class UdiEventErrorImpl extends UdiEventImpl implements UdiEventError {

    private String errorString;

    /**
     * Constructor.
     */
    public UdiEventErrorImpl() {
        super(EventType.ERROR);
    }

    /**
     * @param errorString the error string
     */
    public void setErrorString(String errorString) {
        this.errorString = errorString;
    }

    /**
     * @return the error string describing the error
     */
    @Override
    public String getErrorString() {
        return errorString;
    }

    @Override
    public void accept(UdiEventVisitor visitor) {
        visitor.visit(this);
    }
}
