
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

package net.libudi.api.jni.impl;

import java.util.Arrays;
import java.util.List;

import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

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

    private final UdiProcessManager procManager;

    /**
     * Constructor.
     *
     * @param handle the native handle
     * @param udiLibrary the handle to the udi library
     * @param procManager the process manager that created this process
     */
    public UdiProcessImpl(Pointer handle, UdiLibrary udiLibrary, UdiProcessManager procManager) {
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
        return null;
    }

    @Override
    public boolean isMultithreadCapable() {
        return false;
    }

    @Override
    public UdiThread getInitialThread() {
        return null;
    }

    @Override
    public void continueProcess() throws UdiException {
        int nativeResult = udiLibrary.continue_process(handle);
        UdiException ex = UdiError.toException(nativeResult, udiLibrary.get_last_error_message(handle));

        if ( ex != null ) throw ex;
    }

    @Override
    public void refreshState() throws UdiException {
    }

    @Override
    public void readMemory(byte[] data, UnsignedLong sourceAddr) throws UdiException {
    }

    @Override
    public void writeMemory(byte[] data, UnsignedLong destAddr) throws UdiException {
    }

    @Override
    public void createBreakpoint(UnsignedLong brkptAddr) throws UdiException {
    }

    @Override
    public void installBreakpoint(UnsignedLong brkptAddr) throws UdiException {
    }

    @Override
    public void removeBreakpoint(UnsignedLong brkptAddr) throws UdiException {
    }

    @Override
    public void deleteBreakpoint(UnsignedLong brkptAddr) throws UdiException {
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
    public void close() throws Exception {
    }
}
