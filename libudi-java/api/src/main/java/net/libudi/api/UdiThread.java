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
     * Retrieves the program counter/instruction pointer for the current thread.
     *
     * @return the PC for the current thread
     *
     * @throws UdiException on error
     */
    long getPC() throws UdiException;

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

}
