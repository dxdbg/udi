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
import com.sun.jna.ptr.PointerByReference;

/**
 * The native interface to libudi
 *
 * @author mcnulty
 */
public interface UdiLibrary extends Library {

    /** The library name */
    public static final String LIBRARY_NAME = "udi";

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
     * @return the result
     */
    int continue_process(Pointer process);

    /**
     * @param process the process
     * @return the initial thread for the process
     */
    Pointer get_initial_thread(Pointer process);

    /**
     * The last error message recorded for the specified process
     *
     * @param process the process
     *
     * @return the error message
     */
    String get_last_error_message(Pointer process);

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
