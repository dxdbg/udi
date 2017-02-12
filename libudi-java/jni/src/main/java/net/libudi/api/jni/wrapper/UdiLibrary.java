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
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

/**
 * The native interface to libudi
 */
public interface UdiLibrary extends Library {

    /** The library name */
    String LIBRARY_NAME = "udi";

    // Processes //

    /**
     * Creates a process to be debugged
     *
     * @param executablePath the path to the executable
     * @param argv the arguments to the process
     * @param envp the environment for the process (name=value pair format)
     * @param config the configuration for the process
     * @param process the process (populated on success)
     *
     * @return the result of the operation
     */
    UdiNativeError create_process(String executablePath,
                                  String[] argv,
                                  String[] envp,
                                  UdiNativeProcConfig config,
                                  PointerByReference process);

    /**
     * Frees the process
     *
     * @param process the process
     */
    void free_process(Pointer process);

    /**
     * Continues a process
     *
     * @param process the process
     *
     * @return the result of the operation
     */
    UdiNativeError continue_process(Pointer process);

    /**
     * Refreshes the state of all threads in the process
     *
     * @param process the process
     *
     * @return the result of the operation
     */
    UdiNativeError refresh_state(Pointer process);

    /**
     * Creates a breakpoint in the process
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the result of the operation
     */
    UdiNativeError create_breakpoint(Pointer process, long address);

    /**
     * Installs a breakpoint into the process' memory
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the result of the operation
     */
    UdiNativeError install_breakpoint(Pointer process, long address);

    /**
     * Remove a breakpoint from the process' memory
     *
     * @param process the process
     * @param address the address of the breakpoint
     *
     * @return the result of the operation
     */
    UdiNativeError remove_breakpoint(Pointer process, long address);

    /**
     * Deletes the breakpoint at the specified address
     *
     * @param process the process
     * @param address the address
     *
     * @return the result of the operation
     */
    UdiNativeError delete_breakpoint(Pointer process, long address);

    /**
     * Read the memory
     *
     * @param process the process
     * @param dst the destination buffer
     * @param size the size of data to read
     * @param address the address of the data to read
     *
     * @return the error code of the operation
     */
    UdiNativeError read_mem(Pointer process, Pointer dst, int size, long address);

    /**
     * Write the memory
     *
     * @param process the process
     * @param src the source for the data to write
     * @param size the size of the data to write
     * @param address the address of the data to write
     *
     * @return the error code of the operation
     */
    UdiNativeError write_mem(Pointer process, Pointer src, int size, long address);

    /**
     * Gets the PID for the process
     *
     * @param process the process
     * @param output the output
     *
     * @return the result of the operation
     */
    UdiNativeError get_proc_pid(Pointer process, IntByReference output);

    /**
     * Gets the architecture for the process
     *
     * @param process the process
     * @param output the output
     *
     * @return the result of the operation
     */
    UdiNativeError get_proc_architecture(Pointer process, IntByReference output);

    /**
     * @param process the process
     * @param output the output
     *
     * @return the result of the operation
     */
    UdiNativeError get_multithread_capable(Pointer process, IntByReference output);

    /**
     * @param process the process
     * @param output the output
     *
     * @return the initial thread for the process
     */
    UdiNativeError get_initial_thread(Pointer process, PointerByReference output);

    /**
     * @param process the process
     * @param output true if the process is in a running state. That is, the process had been continued and no events
     * have been received for the process.
     *
     * @return the result of the operation
     */
    UdiNativeError is_running(Pointer process, IntByReference output);

    /**
     * @param process the process
     * @param output true if the process has terminated and is longer accessible.
     *
     * @return the result of the operation
     */
    UdiNativeError is_terminated(Pointer process, IntByReference output);

    // Threads //

    /**
     * Gets the id for the thread
     *
     * @param thread the thread
     * @param output the id
     *
     * @return the result of the operation
     */
    UdiNativeError get_tid(Pointer thread, LongByReference output);

    /**
     * @param thread the thread
     * @param output the current state of the thread
     *
     * @return the result of the operation
     */
    UdiNativeError get_state(Pointer thread, IntByReference output);

    /**
     * Gets the next thread in the parent process
     *
     * @param process the process
     * @param thread the thread
     * @param output the output
     *
     * @return the next thread
     */
    UdiNativeError get_next_thread(Pointer process,
                                   Pointer thread,
                                   PointerByReference output);

    /**
     * Sets the running state of the thread to resumed
     *
     * @param thread the thread
     *
     * @return the error code of the operation
     */
    UdiNativeError resume_thread(Pointer thread);

    /**
     * Sets the running state of the thread to suspended
     *
     * @param thread the thread
     *
     * @return the error code of the operation
     */
    UdiNativeError suspend_thread(Pointer thread);

    /**
     * Sets the single step setting of the thread
     *
     * @param thread the thread
     * @param enable true if single step should be enabled
     *
     * @return the error code of the operation
     */
    UdiNativeError set_single_step(Pointer thread, boolean enable);

    /**
     * @param thread the thread
     * @param output the current single step setting
     *
     * @return the error code of the operation
     */
    UdiNativeError get_single_step(Pointer thread, IntByReference output);

    /**
     * Read a register in the specified thread
     *
     * @param thread the thread
     * @param register the register to access
     * @param value the destination or source of the register value
     *
     * @return the error code of the operation
     */
    UdiNativeError read_register(Pointer thread, int register, LongByReference value);

    /**
     * Write a register in the specified thread
     *
     * @param thread the thread
     * @param register the register to access
     * @param value the source of the register value
     *
     * @return the error code of the operation
     */
    UdiNativeError write_register(Pointer thread, int register, long value);

    /**
     * Gets the program counter for the specified thread
     *
     * @param thread the thread
     * @param value the destination for the value
     *
     * @return the error code of the operation
     */
    UdiNativeError get_pc(Pointer thread, LongByReference value);

    /**
     * Gets the next instruction to be executed by the specified thread
     *
     * @param thread the thread
     * @param value the destination for the value
     *
     * @return the error code of the operation
     */
    UdiNativeError get_next_instruction(Pointer thread, LongByReference value);

    // Events //

    /**
     * Waits for events to occur in the specified processes
     *
     * @param procs the processes
     * @param numProcs the number of processes in the array
     * @param output the native events
     *
     * @return the error code of the operation
     */
    UdiNativeError wait_for_events(Pointer[] procs, int numProcs, PointerByReference output);

    /**
     * Frees the events returned by wait_for_events
     *
     * @param event_list the event list
     */
    void free_event_list(UdiNativeEvent event_list);
}
