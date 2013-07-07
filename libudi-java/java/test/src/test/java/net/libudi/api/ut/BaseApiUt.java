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

    protected static final String TEST_EXECS = "build/tests/bin";

    protected static final String ROOT_DIR = System.getProperty("java.io.tmpdir");

    protected static final String SIMPLE_EXECUTABLE = "simple";

    private final UdiProcessConfig config;

    /**
     * Constructor.
     */
    public BaseApiUt() {
        config = new UdiProcessConfig();
        config.setRootDir(Paths.get(ROOT_DIR, "test-udi"));
    }

    /**
     * Tests the createProcess method
     *
     * @throws UdiException on error
     */
    @Test
    public void testCreateProcess() throws UdiException {

        UdiProcessManager procManager = getProcessManager();

        UdiProcess process = procManager.createProcess(Paths.get(TEST_EXECS, SIMPLE_EXECUTABLE),
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
