/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api;

import java.util.HashMap;
import java.util.Map;

/**
 * Enumeration encapsulating all registers accessible by UDI
 *
 * @author mcnulty
 */
public enum Register {

    // X86 registers
    X86_MIN,
    X86_GS,
    X86_FS,
    X86_ES,
    X86_DS,
    X86_EDI,
    X86_ESI,
    X86_EBP,
    X86_ESP,
    X86_EBX,
    X86_EDX,
    X86_ECX,
    X86_EAX,
    X86_CS,
    X86_SS,
    X86_EIP,
    X86_FLAGS,
    X86_ST0,
    X86_ST1,
    X86_ST2,
    X86_ST3,
    X86_ST4,
    X86_ST5,
    X86_ST6,
    X86_ST7,
    X86_MAX,

    // X86_64 registers
    X86_64_MIN,
    X86_64_R8,
    X86_64_R9,
    X86_64_R10,
    X86_64_R11,
    X86_64_R12,
    X86_64_R13,
    X86_64_R14,
    X86_64_R15,
    X86_64_RDI,
    X86_64_RSI,
    X86_64_RBP,
    X86_64_RBX,
    X86_64_RDX,
    X86_64_RAX,
    X86_64_RCX,
    X86_64_RSP,
    X86_64_RIP,
    X86_64_CSGSFS,
    X86_64_FLAGS,
    X86_64_ST0,
    X86_64_ST1,
    X86_64_ST2,
    X86_64_ST3,
    X86_64_ST4,
    X86_64_ST5,
    X86_64_ST6,
    X86_64_ST7,
    X86_64_XMM0,
    X86_64_XMM1,
    X86_64_XMM2,
    X86_64_XMM3,
    X86_64_XMM4,
    X86_64_XMM5,
    X86_64_XMM6,
    X86_64_XMM7,
    X86_64_XMM8,
    X86_64_XMM9,
    X86_64_XMM10,
    X86_64_XMM11,
    X86_64_XMM12,
    X86_64_XMM13,
    X86_64_XMM14,
    X86_64_XMM15,
    X86_64_MAX;

    private static Map<Integer, Register> regsByIndex = null;

    public static Register fromIndex(int index) {
        if (regsByIndex == null) {
            synchronized (Register.class) {
                if (regsByIndex == null) {
                    regsByIndex = new HashMap<>();

                    // Build the collection
                    int i = 0;
                    for (Register reg : values()) {
                        regsByIndex.put(i, reg);
                        i++;
                    }
                }
            }
        }

        return regsByIndex.get(index);
    }

    /**
     * @return the id for the register
     */
    public int getIndex() {

        // Currently, this is the same as ordinal but this could change in the future
        return ordinal();
    }
}
