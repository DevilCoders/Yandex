package ru.yandex.ci.tms.test.temporal;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;

import ru.yandex.ci.tms.test.EntireTestDisplacement;

//TODO remove after complete temporal switch
public class TemporalEntireTestDisplacement extends EntireTestDisplacement {
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
