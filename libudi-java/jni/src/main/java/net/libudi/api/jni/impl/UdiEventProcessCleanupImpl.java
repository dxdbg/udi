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
import net.libudi.api.event.UdiEventProcessCleanup;
import net.libudi.api.event.UdiEventVisitor;

/**
 * Implementation of UdiEventProcessCleanup
 *
 * @author mcnulty
 */
public class UdiEventProcessCleanupImpl extends UdiEventImpl implements UdiEventProcessCleanup {

    /**
     * Constructor.
     */
    public UdiEventProcessCleanupImpl() {
        super(EventType.PROCESS_CLEANUP);
    }

    @Override
    public void accept(UdiEventVisitor visitor) {
        visitor.visit(this);
    }
}
