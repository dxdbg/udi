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
 * An interface for a UdiEvent of type ERROR
 *
 * @author mcnulty
 */
public interface UdiEventError extends UdiEvent {

    /**
     * @return the error string describing the error
     */
    String getErrorString();
}
