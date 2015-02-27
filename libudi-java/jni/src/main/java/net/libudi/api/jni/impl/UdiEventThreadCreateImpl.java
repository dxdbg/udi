/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import net.libudi.api.UdiThread;
import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEventThreadCreate;
import net.libudi.api.event.UdiEventVisitor;

/**
 * @author mcnulty
 */
public class UdiEventThreadCreateImpl extends UdiEventImpl implements UdiEventThreadCreate {

    private UdiThread newThread;

    /**
     * Constructor.
     */
    public UdiEventThreadCreateImpl() {
        super(EventType.THREAD_CREATE);
    }

    /**
     * @param newThread the newThread value
     */
    public void setNewThread(UdiThread newThread) {
        this.newThread = newThread;
    }

    @Override
    public UdiThread getNewThread() {
        return newThread;
    }

    @Override
    public void accept(UdiEventVisitor visitor) {
        visitor.visit(this);
    }
}
