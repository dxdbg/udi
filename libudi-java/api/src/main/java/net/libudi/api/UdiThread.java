/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api;

import net.libudi.api.exceptions.UdiException;

/**
 * An interface that provides methods to control and query a debuggee thread via libudi.
 *
 * <p>
 *     All methods require the process to be in the stopped state unless otherwise stated
 * </p>
 *
 * @author mcnulty
 */
public interface UdiThread {

    /**
     * This method does not require that the parent process be stopped
     *
     * @return the id for the thread
     */
    long getTid();

    /**
     * This method does not require that the parent process be stopped
     *
     * @return the parent process for the thread
     */
    UdiProcess getParentProcess();

    /**
     * This method does not require that the parent process be stopped
     *
     * @return the state for the thread
     */
    ThreadState getState();

    /**
     * This method links this thread to another thread in the same process. This interface does not enforce any ordering
     * constraints on the results returned from this method, just as long as all threads are reachable from the initial
     * thread for a process.
     *
     * @return the "next" thread, null if there is no "next" thread
     */
    UdiThread getNextThread();

    /**
     * Retrieves the program counter/instruction pointer for this thread.
     *
     * @return the PC for this thread
     *
     * @throws UdiException on error
     */
    long getPC() throws UdiException;

    /**
     * Retrieves the program counter/instruction pointer for next instruction for this thread
     *
     * @return the next instruction for this thread
     *
     * @throws UdiException on error
     */
    long getNextPC() throws UdiException;

    /**
     * Retrieves the value of the specified register
     *
     * @param reg the register
     *
     * @return the register value
     *
     * @throws UdiException on error
     */
    long readRegister(Register reg) throws UdiException;

    /**
     * Writes the value of the specified register
     *
     * @param reg the register
     * @param value the value to write into the register
     *
     * @throws UdiException on error
     */
    void writeRegister(Register reg, long value) throws UdiException;

    /**
     * Changes the state of the thread to running
     *
     * @throws UdiException on error
     */
    void resume() throws UdiException;

    /**
     * Changes the state of the thread to suspended
     *
     * @throws UdiException on error
     */
    void suspend() throws UdiException;

    /**
     * Changes the value of the single step setting for the thread
     *
     * @param singleStep the single step setting
     *
     * @throws UdiException on error
     */
    void setSingleStep(boolean singleStep) throws UdiException;

    /**
     * @return the single step setting
     */
    boolean getSingleStep();
}
