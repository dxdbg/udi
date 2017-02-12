/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import net.libudi.api.UdiProcess;
import net.libudi.api.UdiThread;
import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEvent;
import net.libudi.api.event.UdiEventVisitor;

/**
 * Implementation of UdiEvent
 */
public abstract class UdiEventImpl implements UdiEvent {

    private final EventType eventType;

    private UdiProcess process;

    private UdiThread thread;

    private Object userData;

    /**
     * Constructor.
     *
     * @param eventType the event type
     */
    public UdiEventImpl(EventType eventType) {
        this.eventType = eventType;
    }

    /**
     * @param process the process
     */
    public void setProcess(UdiProcess process) {
        this.process = process;
    }

    /**
     * @param thread the thread
     */
    public void setThread(UdiThread thread) {
        this.thread = thread;
    }

    @Override
    public EventType getEventType() {
        return eventType;
    }

    @Override
    public UdiProcess getProcess() {
        return process;
    }

    @Override
    public UdiThread getThread() {
        return thread;
    }

    @Override
    public Object getUserData() {
        return userData;
    }

    @Override
    public void setUserData(Object userData) {
        this.userData = userData;
    }
}
