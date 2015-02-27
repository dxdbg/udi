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
 * Enum for the type of a UdiEvent
 *
 * @author mcnulty
 */
public enum EventType {

    /** An error event */
    ERROR,

    /** A thread received a signal (likely POSIX-specific) */
    SIGNAL,

    /** A thread has hit a breakpoint */
    BREAKPOINT,

    /** A thread has been created */
    THREAD_CREATE,

    /** A thread has exited/died */
    THREAD_DEATH,

    /** A process has exited */
    PROCESS_EXIT,

    /** A process has forked */
    PROCESS_FORK,

    /** A process has execed */
    PROCESS_EXEC,

    /** A thread has completed a single step */
    SINGLE_STEP,

    /** The process has terminated and any cleanup should be performed */
    PROCESS_CLEANUP;

    /**
     * Gets the EventType that corresponds to the specified index
     *
     * @param index the index
     *
     * @return the EventType
     */
    public static EventType fromIndex(int index) {
        switch(index) {
            case 0:
                return ERROR;
            case 1:
                return SIGNAL;
            case 2:
                return BREAKPOINT;
            case 3:
                return THREAD_CREATE;
            case 4:
                return THREAD_DEATH;
            case 5:
                return PROCESS_EXIT;
            case 6:
                return PROCESS_FORK;
            case 7:
                return PROCESS_EXEC;
            case 8:
                return SINGLE_STEP;
            case 9:
                return PROCESS_CLEANUP;
            default:
                throw new IllegalArgumentException("No EventType with index " + index);
        }
    }

    public int getIndex() {
        return ordinal();
    }
}
