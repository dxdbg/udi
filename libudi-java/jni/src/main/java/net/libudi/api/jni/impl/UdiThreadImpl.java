/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

import net.libudi.api.Register;
import net.libudi.api.ThreadState;
import net.libudi.api.UdiProcess;
import net.libudi.api.UdiThread;
import net.libudi.api.exceptions.UdiException;
import net.libudi.api.jni.wrapper.UdiLibrary;
import net.libudi.api.jni.wrapper.UdiNativeError;

/**
 * Implementation of UdiThread that uses libudi
 */
public class UdiThreadImpl implements UdiThread {

    private final Pointer handle;

    private final UdiProcessImpl parentProcess;

    private final UdiLibrary udiLibrary;

    private final UdiProcessManagerImpl procManager;

    /**
     * Constructor.
     *
     * @param handle the handle
     * @param parentProcess the parent process
     * @param procManager the UdiProcessManager
     * @param udiLibrary the library
     */
    public UdiThreadImpl(Pointer handle,
                         UdiProcessImpl parentProcess,
                         UdiProcessManagerImpl procManager,
                         UdiLibrary udiLibrary) {
        this.handle = handle;
        this.parentProcess = parentProcess;
        this.procManager = procManager;
        this.udiLibrary = udiLibrary;
    }

    @Override
    public long getTid() throws UdiException {

        parentProcess.checkNotClosed();

        LongByReference output = new LongByReference();
        try (UdiNativeError error = udiLibrary.get_tid(handle, output)) {
            error.checkException();
            return output.getValue();
        }
    }

    @Override
    public UdiProcess getParentProcess() {
        return parentProcess;
    }

    @Override
    public ThreadState getState() throws UdiException {

        parentProcess.checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.get_state(handle, output)) {
            error.checkException();
            return ThreadState.fromIndex(output.getValue());
        }
    }

    @Override
    public UdiThread getNextThread() throws UdiException {

        parentProcess.checkNotClosed();

        PointerByReference output = new PointerByReference();

        try (UdiNativeError error = udiLibrary.get_next_thread(parentProcess.getHandle(),
                                                               handle,
                                                               output))
        {
            error.checkException();
            return procManager.getThread(output.getValue());
        }
    }

    @Override
    public long getPC() throws UdiException {

        parentProcess.checkNotClosed();

        LongByReference output = new LongByReference();

        try (UdiNativeError error = udiLibrary.get_pc(handle, output)) {
            error.checkException();
            return output.getValue();
        }
    }

    @Override
    public long getNextPC() throws UdiException {

        parentProcess.checkNotClosed();

        LongByReference output = new LongByReference();

        try (UdiNativeError error = udiLibrary.get_next_instruction(handle, output)) {
            error.checkException();
            return output.getValue();
        }
    }

    @Override
    public long readRegister(Register reg) throws UdiException {

        parentProcess.checkNotClosed();

        LongByReference output = new LongByReference();

        try (UdiNativeError error = udiLibrary.read_register(handle, reg.getIndex(), output)) {
            error.checkException();
            return output.getValue();
        }
    }

    @Override
    public void writeRegister(Register reg, long value) throws UdiException {

        parentProcess.checkNotClosed();

        try (UdiNativeError error = udiLibrary.write_register(handle, reg.getIndex(), value)) {
            error.checkException();
        }
    }

    @Override
    public void resume() throws UdiException {

        parentProcess.checkNotClosed();

        try (UdiNativeError error = udiLibrary.resume_thread(handle)) {
            error.checkException();
        }
    }

    @Override
    public void suspend() throws UdiException {

        parentProcess.checkNotClosed();

        try (UdiNativeError error = udiLibrary.suspend_thread(handle)) {
            error.checkException();
        }
    }

    @Override
    public void setSingleStep(boolean singleStep) throws UdiException {

        parentProcess.checkNotClosed();

        try (UdiNativeError error = udiLibrary.set_single_step(handle, singleStep)) {
            error.checkException();
        }
    }

    @Override
    public boolean getSingleStep() throws UdiException {

        parentProcess.checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.get_single_step(handle, output)) {
            error.checkException();
            return output.getValue() != 0 ? true : false;
        }
    }

    Pointer getHandle() {
        return handle;
    }
}
