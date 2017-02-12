/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.event;

import net.libudi.api.UdiThread;

/**
 * An interface for an event with type THREAD_CREATE
 */
public interface UdiEventThreadCreate extends UdiEvent {

    /**
     * @return the thread that was created
     */
    UdiThread getNewThread();
}
