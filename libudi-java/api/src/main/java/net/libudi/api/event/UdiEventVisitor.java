/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.event;

/**
 * Visitor for dispatching events for handling
 */
public interface UdiEventVisitor {

    /**
     * @param breakpointEvent the breakpoint event
     */
    void visit(UdiEventBreakpoint breakpointEvent);

    /**
     * @param errorEvent the error event
     */
    void visit(UdiEventError errorEvent);

    /**
     * @param processExitEvent the process event
     */
    void visit(UdiEventProcessExit processExitEvent);

    /**
     * @param threadCreateEvent the thread create event
     */
    void visit(UdiEventThreadCreate threadCreateEvent);

    /**
     * @param processCleanup the process cleanup event
     */
    void visit(UdiEventProcessCleanup processCleanup);
}
