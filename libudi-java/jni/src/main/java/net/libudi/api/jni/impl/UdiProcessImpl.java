
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
import com.sun.jna.ptr.LongByReference;

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

/**
 * Implementation of UdiProcess that utilizes the native bindings
 *
 * @author mcnulty
 */
public class UdiProcessImpl implements UdiProcess {

    private final Pointer handle;

    private final UdiLibrary udiLibrary;

    private final UdiProcessManagerImpl procManager;

    private boolean waitingForStart = true;

    private Object userData = null;

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
    public int getPid() {
        return udiLibrary.get_proc_pid(handle);
    }

    @Override
    public Architecture getArchitecture() {
        return Architecture.fromIndex(udiLibrary.get_proc_architecture(handle));
    }

    @Override
    public boolean isMultithreadCapable() {
        return udiLibrary.get_multithread_capable(handle);
    }

    @Override
    public UdiThread getInitialThread() {
        return procManager.getThread(udiLibrary.get_initial_thread(handle));
    }

    @Override
    public boolean isRunning() {
        return udiLibrary.is_running(handle);
    }

    @Override
    public boolean isTerminated() {
        return udiLibrary.is_terminated(handle);
    }

    @Override
    public boolean isWaitingForStart()
    {
        return waitingForStart;
    }

    @Override
    public void continueProcess() throws UdiException {
        checkForException(udiLibrary.continue_process(handle));
        waitingForStart = false;
    }

    @Override
    public void refreshState() throws UdiException {
        checkForException(udiLibrary.refresh_state(handle));
    }

    @Override
    public void readMemory(byte[] data, long sourceAddr) throws UdiException {

        Pointer value = new Memory(data.length);
        checkForException(udiLibrary.mem_access(handle, false, value, data.length, sourceAddr));
        value.read(0, data, 0, data.length);
    }

    @Override
    public void writeMemory(byte[] data, long destAddr) throws UdiException {
        Pointer value = new Memory(data.length);
        value.write(0, data, 0, data.length);
        checkForException(udiLibrary.mem_access(handle, true, value, data.length, destAddr));
    }

    @Override
    public void createBreakpoint(long brkptAddr) throws UdiException {
        checkForException(udiLibrary.create_breakpoint(handle, brkptAddr));
    }

    @Override
    public void installBreakpoint(long brkptAddr) throws UdiException {
        checkForException(udiLibrary.install_breakpoint(handle, brkptAddr));
    }

    @Override
    public void removeBreakpoint(long brkptAddr) throws UdiException {
        checkForException(udiLibrary.remove_breakpoint(handle, brkptAddr));
    }

    @Override
    public void deleteBreakpoint(long brkptAddr) throws UdiException {
        checkForException(udiLibrary.delete_breakpoint(handle, brkptAddr));
    }

    @Override
    public List<UdiEvent> waitForEvents() throws UdiException {
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
        checkForException(udiLibrary.free_process(handle));
    }

    private void checkForException(int errorCode) throws UdiException {
        UdiException ex = UdiError.toException(errorCode, udiLibrary.get_last_error_message(handle));
        if (ex != null) throw ex;
    }
}
