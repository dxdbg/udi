/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
