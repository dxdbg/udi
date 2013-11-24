/*
 * Copyright (c) 2011-2013, Dan McNulty
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the UDI project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package net.libudi.api;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import net.libudi.api.event.UdiEvent;
import net.libudi.api.exceptions.UdiException;

/**
 * An interface that provides a mechanism to create or attach to debuggee processes and wait for events in a collection
 * of processes.
 *
 * @author mcnulty
 */
public interface UdiProcessManager {

    /**
     * Creates a debuggee process that can be controlled, modified and queried using UDI
     *
     * @param executable the full path to the executable for the process
     * @param args the command line arguments to the process (ala UNIX execve)
     * @param env the environment for the process (ala UNIX execve)
     * @param config the configuration for the process
     *
     * @return the created UdiProcess (never null)
     *
     * @throws UdiException when the process cannot be created
     */
    UdiProcess createProcess(Path executable,
            String[] args,
            Map<String, String> env,
            UdiProcessConfig config)
            throws UdiException;

    /**
     * Wait for events to occur in the specified processes
     *
     * @param processes the processes in which to wait for events
     *
     * @return the events that occurred in the processes
     *
     * @throws UdiException on error
     */
    List<UdiEvent> waitForEvents(List<UdiProcess> processes) throws UdiException;

}
