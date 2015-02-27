/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.exceptions;

import net.libudi.api.event.UdiEvent;

/**
 * Thrown to indicate that an unexpected event was encountered
 *
 * @author mcnulty
 */
public class UnexpectedEventException extends UdiException {

    private static final long serialVersionUID = -3286882776335457781L;

    private final UdiEvent event;

    /**
     * Constructor.
     *
     * @param event the unexpected event
     */
    public UnexpectedEventException(UdiEvent event) {
        super("Received unexpected event of type " + event.getEventType() + ": " + event);
        this.event = event;
    }

    /**
     * @return the event
     */
    public UdiEvent getEvent() {
        return event;
    }
}
