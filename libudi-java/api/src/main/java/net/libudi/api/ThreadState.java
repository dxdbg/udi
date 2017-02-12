/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api;

/**
 * The state for a UdiThread
 */
public enum ThreadState {

    /** The thread is in the running state */
    RUNNING,

    /** The thread is in the suspended state */
    SUSPENDED;

    public static ThreadState fromIndex(int index) {
        switch (index) {
            case 0:
                return RUNNING;
            case 1:
                return SUSPENDED;
            default:
                throw new IllegalArgumentException("Unknown state with id " + index);
        }
    }

    public int getIndex() {
        return ordinal();
    }
}
