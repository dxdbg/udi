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
import net.libudi.api.event.UdiEventBreakpoint;
import net.libudi.api.event.UdiEventVisitor;

/**
 * Implementation of UdiEventBreakpoint
 *
 * @author mcnulty
 */
public class UdiEventBreakpointImpl extends UdiEventImpl implements UdiEventBreakpoint {

    private long address;

    /**
     * Constructor.
     */
    public UdiEventBreakpointImpl() {
        super(EventType.BREAKPOINT);
    }

    /**
     * @param address the address to set
     */
    public void setAddress(long address) {
        this.address = address;
    }

    @Override
    public long getAddress() {
        return address;
    }

    @Override
    public void accept(UdiEventVisitor visitor) {
        visitor.visit(this);
    }
}
