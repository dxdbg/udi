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
import com.sun.jna.ptr.LongByReference;

import net.libudi.api.Register;
import net.libudi.api.ThreadState;
import net.libudi.api.UdiProcess;
import net.libudi.api.UdiProcessManager;
import net.libudi.api.UdiThread;
import net.libudi.api.exceptions.UdiException;
import net.libudi.api.jni.wrapper.UdiError;
import net.libudi.api.jni.wrapper.UdiLibrary;

/**
 * Implementation of UdiThread that uses libudi
 *
 * @author mcnulty
 */
public class UdiThreadImpl implements UdiThread {

    private final Pointer handle;

    private final UdiLibrary udiLibrary;

    private final UdiProcessManagerImpl procManager;

    /**
     * Constructor.
     *
     * @param handle the handle
     * @param procManager the UdiProcessManager
     * @param udiLibrary the library
     */
    public UdiThreadImpl(Pointer handle, UdiProcessManagerImpl procManager, UdiLibrary udiLibrary) {
        this.handle = handle;
        this.procManager = procManager;
        this.udiLibrary = udiLibrary;
    }

    @Override
    public long getTid() {
        return udiLibrary.get_tid(handle);
    }

    @Override
    public UdiProcess getParentProcess() {
        return procManager.getProcess(udiLibrary.get_process(handle));
    }

    @Override
    public ThreadState getState() {
        return ThreadState.fromIndex(udiLibrary.get_state(handle));
    }

    @Override
    public UdiThread getNextThread() {
        return procManager.getThread(udiLibrary.get_next_thread(handle));
    }

    @Override
    public long getPC() throws UdiException {

        LongByReference value = new LongByReference();

        checkForException(udiLibrary.get_pc(handle, value));

        return value.getValue();
    }

    @Override
    public long getNextPC() throws UdiException {
        LongByReference value = new LongByReference();

        checkForException(udiLibrary.get_next_instruction(handle, value));

        return value.getValue();
    }

    @Override
    public long readRegister(Register reg) throws UdiException {

        LongByReference value = new LongByReference();

        registerAccess(reg, 0, value);

        return value.getValue();
    }

    @Override
    public void writeRegister(Register reg, long value) throws UdiException {

        LongByReference newValue = new LongByReference(value);

        registerAccess(reg, 1, newValue);
    }

    private void registerAccess(Register reg, int write, LongByReference value) throws UdiException {

        checkForException(udiLibrary.register_access(handle, write, reg.getIndex(), value));
    }

    @Override
    public void resume() throws UdiException {
        checkForException(udiLibrary.resume_thread(handle));
    }

    @Override
    public void suspend() throws UdiException {
        checkForException(udiLibrary.suspend_thread(handle));
    }

    @Override
    public void setSingleStep(boolean singleStep) throws UdiException {
        checkForException(udiLibrary.set_single_step(handle, singleStep));
    }

    @Override
    public boolean getSingleStep() {
        return udiLibrary.get_single_step(handle);
    }

    private void checkForException(int errorCode) throws UdiException {
        UdiException ex = UdiError.toException(errorCode, udiLibrary.get_last_error_message(udiLibrary.get_process(handle)));
        if (ex != null) throw ex;
    }
}
