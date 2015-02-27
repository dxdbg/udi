/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package net.libudi.api.jni.impl;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

import net.libudi.api.UdiProcess;
import net.libudi.api.UdiProcessConfig;
import net.libudi.api.UdiProcessManager;
import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEvent;
import net.libudi.api.exceptions.InternalLibraryException;
import net.libudi.api.exceptions.UdiException;
import net.libudi.api.jni.wrapper.CLibrary;
import net.libudi.api.jni.wrapper.UdiError;
import net.libudi.api.jni.wrapper.UdiLibrary;
import net.libudi.api.jni.wrapper.UdiNativeEvent;
import net.libudi.api.jni.wrapper.UdiNativeEventBreakpoint;
import net.libudi.api.jni.wrapper.UdiNativeEventError;
import net.libudi.api.jni.wrapper.UdiNativeEventProcessExit;
import net.libudi.api.jni.wrapper.UdiNativeEventThreadCreate;
import net.libudi.api.jni.wrapper.UdiNativeProcConfig;

/**
 * Implementation of UdiProcessManager that utilizes the native libudi bindings
 *
 * @author mcnulty
 */
public class UdiProcessManagerImpl implements UdiProcessManager {

    /** The native library handle */
    private final UdiLibrary nativeLibrary;

    /** The native libc handle */
    private final CLibrary nativeCLibrary;

    /** Used to limit instantiations to one instance */
    private static boolean instantiated = false;

    // These are necessary because a Java object cannot be maintained by the libudi (even though it provides for
    // a way to store a opaque pointer with each process and thread)

    private final Map<Pointer, UdiProcessImpl> procsByPointer = new HashMap<>();

    private final Map<Pointer, UdiThreadImpl> threadsByPointer = new HashMap<>();

    /**
     * Constructor.
     *
     * Loads the underlying native library.
     */
    public UdiProcessManagerImpl() {
        synchronized (UdiProcessManagerImpl.class) {
            if (!instantiated) {
                instantiated = true;
            }else{
                throw new IllegalStateException("Only one instance of " + UdiProcessManagerImpl.class.getSimpleName()
                        +  " allowed.");
            }
        }

        Map<String, Boolean> options = new HashMap<>();
        options.put(Library.OPTION_ALLOW_OBJECTS, Boolean.TRUE);
        nativeLibrary = (UdiLibrary) Native.loadLibrary(UdiLibrary.LIBRARY_NAME, UdiLibrary.class, options);
        nativeCLibrary = (CLibrary) Native.loadLibrary(CLibrary.LIBRARY_NAME, CLibrary.class);
    }

    public UdiProcessImpl getProcess(Pointer process) {
        return procsByPointer.get(process);
    }

    public UdiThreadImpl getThread(Pointer thread) {
        return threadsByPointer.get(thread);
    }

    @Override
    public UdiProcess createProcess(Path executable, String[] args, Map<String, String> env, UdiProcessConfig config)
            throws UdiException
    {
        // Create the environment array
        String[] envp;
        if ( env != null ) {
            envp = new String[env.size()];

            int i = 0;
            for (Map.Entry<String, String> entry : env.entrySet()) {
                envp[i++] = entry.getKey() + "=" + entry.getValue();
            }
        }else{
            envp = null;
        }

        IntByReference errorCode = new IntByReference();
        PointerByReference errMsg = new PointerByReference();

        String[] actualArgs;
        if (args == null) {
            actualArgs = new String[0];
        }else{
            actualArgs = args;
        }

        Pointer handle = nativeLibrary.create_process(executable.toAbsolutePath().toString(), actualArgs, envp,
                new UdiNativeProcConfig(config), errorCode, errMsg);

        if ( handle == null ) {
            // Construct the exception
            String errMsgStr = errMsg.getValue().getString(0);
            nativeCLibrary.free(errMsg.getPointer());

            throw UdiError.toException(errorCode.getValue(), errMsgStr);
        }

        UdiProcessImpl process = new UdiProcessImpl(handle, nativeLibrary, this);
        procsByPointer.put(handle, process);

        Pointer initialThread = nativeLibrary.get_initial_thread(handle);
        if ( initialThread == null ) {
            throw new NativeLibraryException("Failed to determine initial thread for process");
        }
        threadsByPointer.put(initialThread, new UdiThreadImpl(initialThread, this, nativeLibrary));

        return process;
    }

    /**
     * Creates a Java-level UdiEvent from the native event
     *
     * @param event the event
     *
     * @return the Java-level event
     *
     * @throws UdiException on error
     */
    private UdiEventImpl unpackJavaEvent(UdiNativeEvent event) throws UdiException {

        UdiEventImpl eventImpl;
        EventType eventType = EventType.fromIndex(event.event_type);
        switch (eventType) {
            case BREAKPOINT: {
                UdiNativeEventBreakpoint nativeEvBreakpoint = new UdiNativeEventBreakpoint(event.event_data);

                UdiEventBreakpointImpl brkptImpl = new UdiEventBreakpointImpl();
                brkptImpl.setAddress(nativeEvBreakpoint.breakpoint_addr);
                eventImpl = brkptImpl;
                break;
            }
            case ERROR: {
                UdiNativeEventError nativeEvError = new UdiNativeEventError(event.event_data);

                UdiEventErrorImpl errorImpl = new UdiEventErrorImpl();
                errorImpl.setErrorString(nativeEvError.errstr);
                eventImpl = errorImpl;
                break;
            }
            case PROCESS_EXIT: {
                UdiNativeEventProcessExit nativeEvProcExit = new UdiNativeEventProcessExit(event.event_data);

                UdiEventProcessExitImpl procExitImpl = new UdiEventProcessExitImpl();
                procExitImpl.setExitCode(nativeEvProcExit.exit_code);
                eventImpl = procExitImpl;
                break;
            }
            case THREAD_CREATE: {
                UdiNativeEventThreadCreate nativeEvThreadCreate = new UdiNativeEventThreadCreate(event.event_data);

                UdiThreadImpl newThread = new UdiThreadImpl(nativeEvThreadCreate.new_thr, this, nativeLibrary);
                threadsByPointer.put(nativeEvThreadCreate.new_thr, newThread);

                UdiEventThreadCreateImpl threadCreateImpl = new UdiEventThreadCreateImpl();
                threadCreateImpl.setNewThread(newThread);
                eventImpl = threadCreateImpl;
                break;
            }
            case PROCESS_CLEANUP: {
                eventImpl = new UdiEventProcessCleanupImpl();
                break;
            }
            default:
                throw new UdiException(String.format("Unknown event encountered with type '%s'", eventType));
        }

        UdiProcessImpl procImpl = procsByPointer.get(event.proc);
        if ( procImpl == null ) {
            throw new NativeLibraryException("Failed to locate UdiProcess for native udi_process");
        }
        eventImpl.setProcess(procImpl);

        UdiThreadImpl threadImpl = threadsByPointer.get(event.thr);
        if ( threadImpl == null ) {
            throw new NativeLibraryException("Failed to locate UdiThread for native udi_thread");
        }
        eventImpl.setThread(threadImpl);

        return eventImpl;
    }

    @Override
    public List<UdiEvent> waitForEvents(List<UdiProcess> processes) throws UdiException {
        Pointer[] procs = new Pointer[processes.size()];

        int i = 0;
        for (UdiProcess process : processes) {
            procs[i] = ((UdiProcessImpl)process).getHandle();
            i++;
        }

        UdiNativeEvent event = nativeLibrary.wait_for_events(procs, processes.size());

        if ( event == null ) {
            throw new InternalLibraryException("Failed to wait for events");
        }

        List<UdiEvent> events = new ArrayList<>();
        UdiNativeEvent iter = event;
        while (iter != null) {
            events.add(unpackJavaEvent(iter));
            iter = iter.next_event;
        }

        nativeLibrary.free_event_list(event);

        return events;
    }
}
