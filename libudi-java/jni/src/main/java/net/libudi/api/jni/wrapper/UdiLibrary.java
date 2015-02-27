/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
     * Frees the process
     *
     * @param process the process
     *
     * @return the error code for the operation
     */
    int free_process(Pointer process);

    /**
     * Continues a process
     *
     * @param process the process
     *
     * @return the error code for the operation
     */
    int continue_process(Pointer process);

    /**
     * Refreshes the state of all threads in the process
     *
     * @param process the process
     *
     * @return the error code for the operation
     */
    int refresh_state(Pointer process);

    /**
     * Creates a breakpoint in the process
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the error code for the operation
     */
    int create_breakpoint(Pointer process, long address);

    /**
     * Installs a breakpoint into the process' memory
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the error code for the operation
     */
    int install_breakpoint(Pointer process, long address);

    /**
     * Remove a breakpoint from the process' memory
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the error code for the operation
     */
    int remove_breakpoint(Pointer process, long address);

    /**
     * Deletes the breakpoint at the specified address
     *
     * @param process the process
     * @param address the address
     *
     * @return the error code for the operation
     */
    int delete_breakpoint(Pointer process, long address);

    /**
     * Performs a memory access at the specified address
     *
     * @param process the process
     * @param write true if a write should be performed
     * @param value the destination or the source (depending on the value of the write parameter)
     * @param length the length of data to read or write
     * @param address the address of the data
     *
     * @return the error code of the operation
     */
    int mem_access(Pointer process, boolean write, Pointer value, int length, long address);

    /**
     * Gets the PID for the process
     *
     * @param process the process
     *
     * @return the PID
     */
    int get_proc_pid(Pointer process);

    /**
     * Gets the architecture for the process
     *
     * @param process the process
     *
     * @return the architecture
     */
    int get_proc_architecture(Pointer process);

    /**
     * @param process the process
     * @return true if the process is multithread capable
     */
    boolean get_multithread_capable(Pointer process);

    /**
     * @param process the process
     * @return the initial thread for the process
     */
    Pointer get_initial_thread(Pointer process);

    /**
     * @param process the process
     *
     * @return true if the process is in a running state. That is, the process had been continued and no events
     * have been received for the process.
     */
    boolean is_running(Pointer process);

    /**
     * @param process the process
     *
     * @return true if the process has terminated and is longer accessible.
     */
    boolean is_terminated(Pointer process);

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
     * @param thread the thread
     * @return the current state of the thread
     */
    int get_state(Pointer thread);

    /**
     * Gets the next thread in the parent process
     *
     * @param thread the thread
     *
     * @return the next thread
     */
    Pointer get_next_thread(Pointer thread);

    /**
     * Sets the running state of the thread to resumed
     *
     * @param thread the thread
     *
     * @return the error code of the operation
     */
    int resume_thread(Pointer thread);

    /**
     * Sets the running state of the thread to suspended
     *
     * @param thread the thread
     *
     * @return the error code of the operation
     */
    int suspend_thread(Pointer thread);

    /**
     * Sets the single step setting of the thread
     *
     * @param thread the thread
     * @param enable true if single step should be enabled
     *
     * @return the error code of the operation
     */
    int set_single_step(Pointer thread, boolean enable);

    /**
     * @param thread the thread
     *
     * @return the current single step setting
     */
    boolean get_single_step(Pointer thread);

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

    /**
     * Gets the next instruction to be executed by the specified thread
     *
     * @param thread the thread
     * @param value the destination for the value
     *
     * @return the error code of the operation
     */
    int get_next_instruction(Pointer thread, LongByReference value);

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
