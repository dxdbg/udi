package net.libudi.api.ut;

import net.libudi.api.UdiProcessManager;
import net.libudi.api.jni.impl.UdiProcessManagerImpl;

/**
 * Implementation of unit tests for Native implementation of libudi api
 *
 * @author mcnulty
 */
public class NativeApiTest extends BaseApiUt {

    private final UdiProcessManagerImpl impl = new UdiProcessManagerImpl();

    @Override
    protected UdiProcessManager getProcessManager() {
        return impl;
    }
}
