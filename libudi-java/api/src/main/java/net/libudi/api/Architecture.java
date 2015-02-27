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
 * The architecture enum for a UdiProcess
 *
 * @author mcnulty
 */
public enum Architecture {

    /** x86 architecture */
    X86,

    /** x86_64 architecture */
    X86_64;

    public static Architecture fromIndex(int index) {
        switch (index) {
            case 0:
                return X86;
            case 1:
                return X86_64;
            default:
                throw new IllegalArgumentException("Unknown architecture with index " + index);
        }
    }

    public int getIndex() {
        return ordinal();
    }
}
