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
package net.libudi.api.jni.wrapper;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

/**
 * The native interface to libudi
 *
 * @author mcnulty
 */
public interface UdiLibrary extends Library {

    /** The library name */
    public static final String LIBRARY_NAME = "udi";

    // Processes //

    /**
     * Creates a process to be debugged
     *
     * @param executablePath the path to the executable
     * @param argv the arguments to the process
     * @param envp the environment for the process (name=value pair format)
     * @param config the configuration for the process
     * @param errorCode the error code (populated on error)
     * @param errMsg the error message (populated on error)
     *
     * @return the handle the udi_process
     */
    Pointer create_process(String executablePath, String[] argv, String[] envp, UdiNativeProcConfig config,
            IntByReference errorCode, PointerByReference errMsg);

    /**
     * Continues a process
     *
     * @param process the process
     *
     * @return the error code for the operation
     */
    int continue_process(Pointer process);

    /**
     * Gets the PID for the process
     *
     * @param process the process
     *
     * @return the PID
     */
    int get_proc_pid(Pointer process);

    /**
     * @param process the process
     * @return the initial thread for the process
     */
    Pointer get_initial_thread(Pointer process);

    /**
     * @param process the process
     *
     * @return non-zero if the process is in a running state. That is, the process has been continued and no events
     * have been received for the process.
     */
    int is_running(Pointer process);

    /**
     * The last error message recorded for the specified process
     *
     * @param process the process
     *
     * @return the error message
     */
    String get_last_error_message(Pointer process);

    // Threads //

    /**
     * Gets the id for the thread
     *
     * @param thread the thread
     *
     * @return the id
     */
    long get_tid(Pointer thread);

    /**
     * Gets the parent process for a thread
     *
     * @param thread the thread
     *
     * @return the parent process
     */
    Pointer get_process(Pointer thread);

    /**
     * Gets the next thread in the parent process
     *
     * @param thread the thread
     *
     * @return the next thread
     */
    Pointer get_next_thread(Pointer thread);

    /**
     * Access a register in the specified thread
     *
     * @param thread the thread
     * @param write non-zero if the register should be written into
     * @param register the register to access
     * @param value the destination or source of the register value
     *
     * @return the error code of the operation
     */
    int register_access(Pointer thread, int write, int register, LongByReference value);

    /**
     * Gets the program counter for the specified thread
     *
     * @param thread the thread
     * @param value the destination for the value
     *
     * @return the error code of the operation
     */
    int get_pc(Pointer thread, LongByReference value);

    // Events //

    /**
     * Waits for events to occur in the specified processes
     *
     * @param procs the processes
     * @param numProcs the number of processes in the array
     *
     * @return the native events
     */
    UdiNativeEvent wait_for_events(Pointer[] procs, int numProcs);

    /**
     * Frees the events returned by wait_for_events
     *
     * @param event_list the event list
     */
    void free_event_list(UdiNativeEvent event_list);
}
