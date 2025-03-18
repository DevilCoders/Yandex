package ru.yandex.ci.tms.test.temporal;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;

import ru.yandex.ci.tms.test.EntireTestCleanup;

//TODO remove after complete temporal switch
public class TemporalEntireTestCleanup extends EntireTestCleanup {

    @Override
    @BeforeEach
    public void setUp() {
        super.setUp();
        enableTemporal();
    }

    @AfterEach
    void tearDown() {
        ensureOnlyTemporal();
    }
}
