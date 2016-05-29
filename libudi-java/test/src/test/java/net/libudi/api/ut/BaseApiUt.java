/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.ut;

import java.nio.file.Paths;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.junit.Test;

import net.libudi.api.UdiProcess;
import net.libudi.api.UdiProcessConfig;
import net.libudi.api.UdiProcessManager;
import net.libudi.api.event.EventType;
import net.libudi.api.event.UdiEvent;
import net.libudi.api.event.UdiEventProcessExit;
import net.libudi.api.exceptions.UdiException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

/**
 * Base unit test for libudi Java API -- implementations extend this class
 *
 * This test assumes the working directory is set to the base of the udi repository.
 *
 * @author mcnulty
 */
public abstract class BaseApiUt {

    /**
     * @return the actual implementation to test as determined by the base class
     */
    protected abstract UdiProcessManager getProcessManager();

    protected static final String UDI_SRC_ROOT = System.getProperty("udi.src.root");

    protected static final String ROOT_DIR = System.getProperty("java.io.tmpdir");

    protected static final String SIMPLE_EXECUTABLE = "simple";

    protected final String testExecs;

    private final UdiProcessConfig config;

    /**
     * Constructor.
     */
    public BaseApiUt() {
        config = new UdiProcessConfig();
        config.setRootDir(Paths.get(ROOT_DIR, "test-udi"));

        assertNotNull(UDI_SRC_ROOT);
        testExecs = Paths.get(UDI_SRC_ROOT, "build/tests/bin").toAbsolutePath().toString();
    }

    /**
     * Tests the createProcess method
     *
     * @throws UdiException on error
     */
    @Test
    public void testCreateProcess() throws UdiException {

        UdiProcessManager procManager = getProcessManager();

        UdiProcess process = procManager.createProcess(Paths.get(testExecs, SIMPLE_EXECUTABLE),
                new String[0],
                null, // Need to inherit the current environment
                config);
        assertNotEquals(process, null);

        process.continueProcess();

        UdiEvent event = process.waitForEvent(EventType.PROCESS_EXIT);
        assertNotEquals(event, null);
        assertEquals(event.getEventType(), EventType.PROCESS_EXIT);
        assertEquals(((UdiEventProcessExit)event).getExitCode(), 1);

        process.continueProcess();
    }
}
