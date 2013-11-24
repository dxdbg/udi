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

package net.libudi.api;

import java.util.List;

import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEvent;
import net.libudi.api.exceptions.UdiException;
import net.libudi.api.exceptions.UnexpectedEventException;

/**
 * An interface that provides methods to control, modify and query a debuggee process via libudi.
 *
 * <p>
 * All methods require the process to be in the stopped state unless otherwise stated
 * </p>
 *
 * @author mcnulty
 */
public interface UdiProcess extends AutoCloseable {

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the id for the process
     */
    int getPid();

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the architecture for the process
     */
    Architecture getArchitecture();

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return true, if the process is multithread capable
     */
    boolean isMultithreadCapable();

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the initial thread for the process (this can be used to iterate over all the threads)
     */
    UdiThread getInitialThread();

    /**
     * @return true, if the process is running. That is, if the process has been continued and hasn't been stopped by
     * an event yet.
     */
    boolean isRunning();

    /**
     * Continues the process
     *
     * @throws UdiException on error
     */
    void continueProcess() throws UdiException;

    /**
     * Refreshes the state of the process
     *
     * @throws UdiException on error
     */
    void refreshState() throws UdiException;

    /**
     * Reads memory from the debuggee process
     *
     * @param data the destination for the data to read from the debuggee (array specifies length)
     * @param sourceAddr the source address in the debuggee
     *
     * @throws UdiException on error
     */
    void readMemory(byte[] data, long sourceAddr) throws UdiException;

    /**
     * Writes data into memory of the debuggee process.
     *
     * @param data the source for the data to write into the debuggee (array specifies length)
     * @param destAddr the destination address in the debuggee
     *
     * @throws UdiException on error
     */
    void writeMemory(byte[] data, long destAddr) throws UdiException;

    /**
     * Creates a breakpoint at the specified address, but does not install (aka enable) the breakpoint. That is, the
     * breakpoint will not be hit by the debuggee until it is installed
     *
     * @param brkptAddr the address of the breakpoint
     *
     * @throws UdiException on error
     */
    void createBreakpoint(long brkptAddr) throws UdiException;

    /**
     * Installs the breakpoint into the debuggee process
     *
     * @param brkptAddr the address of the breakpoint
     *
     * @throws UdiException on error
     */
    void installBreakpoint(long brkptAddr) throws UdiException;

    /**
     * Removes the breakpoint from the debuggee process (the opposite operation from install)
     *
     * @param brkptAddr the address of the breakpoint
     *
     * @throws UdiException on error
     */
    void removeBreakpoint(long brkptAddr) throws UdiException;

    /**
     * Deletes the breakpoint from the debuggee process (the opposite operation from create)
     *
     * @param brkptAddr the address of the breakpoint
     *
     * @throws UdiException on error
     */
    void deleteBreakpoint(long brkptAddr) throws UdiException;

    /**
     * Waits for events to occur in this process
     *
     * @return the events that occured
     *
     * @throws UdiException on error
     */
    List<UdiEvent> waitForEvents() throws UdiException;

    /**
     * Waits for an event of the specified type to occur
     *
     * @param eventType the event type
     *
     * @throws UnexpectedEventException when an event with a different event type occurs
     * @throws UdiException on general error
     */
    UdiEvent waitForEvent(EventType eventType) throws UnexpectedEventException, UdiException;
}
