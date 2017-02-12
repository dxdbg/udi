
/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.jni.impl;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

import net.libudi.api.Architecture;
import net.libudi.api.UdiProcess;
import net.libudi.api.UdiProcessManager;
import net.libudi.api.UdiThread;
import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEvent;
import net.libudi.api.exceptions.UdiException;
import net.libudi.api.exceptions.UnexpectedEventException;
import net.libudi.api.jni.wrapper.UdiError;
import net.libudi.api.jni.wrapper.UdiLibrary;
import net.libudi.api.jni.wrapper.UdiNativeError;

/**
 * Implementation of UdiProcess that utilizes the native bindings
 */
public class UdiProcessImpl implements UdiProcess {

    private final Pointer handle;

    private final UdiLibrary udiLibrary;

    private final UdiProcessManagerImpl procManager;

    private boolean waitingForStart = true;

    private Object userData = null;

    private boolean closed = false;

    /**
     * Constructor.
     *
     * @param handle the native handle
     * @param udiLibrary the handle to the udi library
     * @param procManager the process manager that created this process
     */
    public UdiProcessImpl(Pointer handle, UdiLibrary udiLibrary, UdiProcessManagerImpl procManager) {
        this.handle = handle;
        this.udiLibrary = udiLibrary;
        this.procManager = procManager;
    }

    /**
     * Package private to allow ProcessManagerImpl to grab the handle
     *
     * @return the handle
     */
    Pointer getHandle() {
        return handle;
    }

    @Override
    public int getPid() throws UdiException {

        checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.get_proc_pid(handle, output)) {
            error.checkException();
            return output.getValue();
        }
    }

    @Override
    public Architecture getArchitecture() throws UdiException {

        checkNotClosed();

        IntByReference output = new IntByReference();

        try (UdiNativeError error = udiLibrary.get_proc_architecture(handle, output)) {
            error.checkException();
            return Architecture.fromIndex(output.getValue());
        }
    }

    @Override
    public boolean isMultithreadCapable() throws UdiException {

        checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.get_multithread_capable(handle, output)) {
            error.checkException();
            return output.getValue() != 0;
        }
    }

    @Override
    public UdiThread getInitialThread() throws UdiException {

        checkNotClosed();

        PointerByReference output = new PointerByReference();
        try (UdiNativeError error = udiLibrary.get_initial_thread(handle, output)) {
            error.checkException();
            return procManager.getThread(output.getValue());
        }
    }

    @Override
    public boolean isRunning() throws UdiException {

        checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.is_running(handle, output)) {
            error.checkException();
            return output.getValue() != 0;
        }
    }

    @Override
    public boolean isTerminated() throws UdiException {

        checkNotClosed();

        IntByReference output = new IntByReference();
        try (UdiNativeError error = udiLibrary.is_terminated(handle, output)) {
            error.checkException();
            return output.getValue() != 0;
        }
    }

    @Override
    public boolean isWaitingForStart()
    {
        return waitingForStart;
    }

    @Override
    public void continueProcess() throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.continue_process(handle)) {
            error.checkException();
            waitingForStart = false;
        }
    }

    @Override
    public void refreshState() throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.refresh_state(handle)) {
            error.checkException();
        }
    }

    @Override
    public void readMemory(byte[] data, long sourceAddr) throws UdiException {

        checkNotClosed();

        Pointer value = new Memory(data.length);
        try (UdiNativeError error = udiLibrary.read_mem(handle, value, data.length, sourceAddr)) {
            error.checkException();
            value.read(0, data, 0, data.length);
        }
    }

    @Override
    public void writeMemory(byte[] data, long destAddr) throws UdiException {

        checkNotClosed();

        Pointer value = new Memory(data.length);
        value.write(0, data, 0, data.length);

        try (UdiNativeError error = udiLibrary.write_mem(handle, value, data.length, destAddr)) {
            error.checkException();
        }
    }

    @Override
    public void createBreakpoint(long brkptAddr) throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.create_breakpoint(handle, brkptAddr)) {
            error.checkException();
        }
    }

    @Override
    public void installBreakpoint(long brkptAddr) throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.install_breakpoint(handle, brkptAddr)) {
            error.checkException();
        }
    }

    @Override
    public void removeBreakpoint(long brkptAddr) throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.remove_breakpoint(handle, brkptAddr)) {
            error.checkException();
        }
    }

    @Override
    public void deleteBreakpoint(long brkptAddr) throws UdiException {

        checkNotClosed();

        try (UdiNativeError error = udiLibrary.delete_breakpoint(handle, brkptAddr)) {
            error.checkException();
        }
    }

    @Override
    public List<UdiEvent> waitForEvents() throws UdiException {

        checkNotClosed();

        return procManager.waitForEvents(Arrays.asList(new UdiProcess[] {this}));
    }

    @Override
    public UdiEvent waitForEvent(EventType eventType) throws UnexpectedEventException, UdiException {

        List<UdiEvent> events = waitForEvents();

        if ( events.size() != 1 || events.get(0).getEventType() != eventType ) {
            throw new UnexpectedEventException(events.get(0));
        }

        return events.get(0);
    }

    @Override
    public Object getUserData()
    {
        return userData;
    }

    @Override
    public void setUserData(Object object)
    {
        this.userData = object;
    }

    @Override
    public void close() throws Exception {
        if (!closed) {
            udiLibrary.free_process(handle);
            closed = true;
        }
    }

    void checkNotClosed() throws UdiException
    {
        if (closed) {
            throw new UdiException("Invalid operation on closed process");
        }
    }
}
