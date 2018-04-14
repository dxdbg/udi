/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package net.libudi.api.ut;

import java.io.IOException;
import java.nio.file.Path;
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
import net.libudi.nativefiletests.NativeFileTestsInfo;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

/**
 * Base unit test for libudi Java API -- implementations extend this class
 */
public abstract class BaseApiUt {

    private static final String ROOT_DIR = System.getProperty("java.io.tmpdir");
    private static final String SIMPLE_BINARY = "simple-debug-noopt-dynamic";
    private static final String NATIVE_FILE_TEST_PATH = "native.file.tests.basePath";
    private static final String RT_LIB_PATH = System.getProperty("udi.native.rtlib.path");

    private final UdiProcessConfig config;
    private final NativeFileTestsInfo nativeFileTestsInfo;

    /**
     * Constructor.
     *
     * @throws IOException on failure to read the native file tests info
     */
    public BaseApiUt() throws IOException
    {
        config = new UdiProcessConfig();
        config.setRootDir(Paths.get(ROOT_DIR, "test-udi"));
        if (RT_LIB_PATH != null) {
            config.setRtLibPath(Paths.get(RT_LIB_PATH + getDynamicLibSuffix()));
        }

        String basePath = System.getProperty(NATIVE_FILE_TEST_PATH);
        assertNotNull(NATIVE_FILE_TEST_PATH + " must be set.", basePath);

        nativeFileTestsInfo = new NativeFileTestsInfo(Paths.get(basePath));
    }

    private String getDynamicLibSuffix() {
        String osName = System.getProperty("os.name");

        if (osName.equalsIgnoreCase("Linux")) {
            return "so";
        }

        if (osName.contains("Mac")) {
            return "dylib";
        }

        throw new IllegalStateException("Unsupported OS " + osName);
    }

    /**
     * Tests the createProcess method
     *
     * @throws UdiException on error
     */
    @Test
    public void testCreateProcess() throws UdiException {

        UdiProcessManager procManager = getProcessManager();

        UdiProcess process = procManager.createProcess(nativeFileTestsInfo.getFirstExecutablePath(SIMPLE_BINARY),
                                                       new String[0],
                                                       null, // Need to inherit the current environment
                                                       config);
        assertNotEquals(process, null);

        process.continueProcess();

        UdiEvent event = process.waitForEvent(EventType.PROCESS_EXIT);
        assertNotEquals(event, null);
        assertEquals(event.getEventType(), EventType.PROCESS_EXIT);
        assertEquals(1, ((UdiEventProcessExit)event).getExitCode());

        process.continueProcess();
    }

    /**
     * @return the actual implementation to test as determined by the base class
     */
    protected abstract UdiProcessManager getProcessManager();
}
