/*
 * Copyright (c) 2011-2013, Dan McNulty
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the UDI project nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
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
