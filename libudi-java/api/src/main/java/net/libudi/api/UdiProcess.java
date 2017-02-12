/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
 */
public interface UdiProcess extends AutoCloseable {

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the id for the process
     *
     * @throws UdiException on error
     */
    int getPid() throws UdiException;

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the architecture for the process
     *
     * @throws UdiException on error
     */
    Architecture getArchitecture() throws UdiException;

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return true, if the process is multithread capable
     *
     * @throws UdiException on error
     */
    boolean isMultithreadCapable() throws UdiException;

    /**
     * This method does not require the process to be in a stopped state
     *
     * @return the initial thread for the process (this can be used to iterate over all the threads)
     *
     * @throws UdiException on error
     */
    UdiThread getInitialThread() throws UdiException;

    /**
     * @return true, if the process is running. That is, if the process has been continued and hasn't been stopped by
     * an event yet.
     *
     * @throws UdiException on error
     */
    boolean isRunning() throws UdiException;

    /**
     * @return true if the process has terminated and is no longer accessible
     *
     * @throws UdiException on error
     */
    boolean isTerminated() throws UdiException;

    /**
     * @return true if the process has been initialized but is waiting for the initial continue
     */
    boolean isWaitingForStart();

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

    /**
     * @return application-specific data that is attached to this process
     */
    Object getUserData();

    /**
     * @param object the application-specific data that is attached to this process
     */
    void setUserData(Object object);
}
