/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.event;

import net.libudi.api.UdiProcess;
import net.libudi.api.UdiThread;

/**
 * An interface representing an event that has occurred in a UdiProcess
 *
 * @author mcnulty
 */
public interface UdiEvent {

    /**
     * @return the type for the event (see corresponding sub-interface)
     */
    EventType getEventType();

    /**
     * @return the process in which the event occurred
     */
    UdiProcess getProcess();

    /**
     * @return the thread in which the thread occurred
     */
    UdiThread getThread();

    /**
     * @return application-specific data that is attached to this event
     */
    Object getUserData();

    /**
     * @param object the application-specific data that is attached to this event
     */
    void setUserData(Object object);

    /**
     * @param visitor the visitor used to process this event
     */
    void accept(UdiEventVisitor visitor);
}
