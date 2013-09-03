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

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.google.common.primitives.UnsignedLong;
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
    // a way to store a opaque pointer with each processs and thread)

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

        Pointer handle = nativeLibrary.create_process(executable.toAbsolutePath().toString(), args, envp,
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
        threadsByPointer.put(initialThread, new UdiThreadImpl(initialThread, nativeLibrary));

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
                brkptImpl.setAddress(UnsignedLong.fromLongBits(nativeEvBreakpoint.breakpoint_addr));
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

                UdiThreadImpl newThread = new UdiThreadImpl(nativeEvThreadCreate.new_thr, nativeLibrary);
                threadsByPointer.put(nativeEvThreadCreate.new_thr, newThread);

                UdiEventThreadCreateImpl threadCreateImpl = new UdiEventThreadCreateImpl();
                threadCreateImpl.setNewThread(newThread);
                eventImpl = threadCreateImpl;
                break;
            }
            default:
                eventImpl = new UdiEventImpl(eventType);
                break;
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
        while (event != null) {
            events.add(unpackJavaEvent(event));
            event = event.next_event;
        }

        nativeLibrary.free_event_list(event);

        return events;
    }
}
