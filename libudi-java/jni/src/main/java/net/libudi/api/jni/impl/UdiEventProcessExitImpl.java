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
import net.libudi.api.event.UdiEventProcessExit;
import net.libudi.api.event.UdiEventVisitor;

/**
 * Implementation of UdiEventProcessExit
 */
public class UdiEventProcessExitImpl extends UdiEventImpl implements UdiEventProcessExit {

    private int exitCode;

    /**
     * Constructor.
     */
    public UdiEventProcessExitImpl() {
        super(EventType.PROCESS_EXIT);
    }

    /**
     * @param exitCode the exit code
     */
    public void setExitCode(int exitCode) {
        this.exitCode = exitCode;
    }

    @Override
    public int getExitCode() {
        return exitCode;
    }

    @Override
    public void accept(UdiEventVisitor visitor) {
        visitor.visit(this);
    }
}
