package ru.yandex.ci.tms.test.temporal;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;

import ru.yandex.ci.tms.test.EntireTestConditional;

//TODO remove after complete temporal switch
public class TemporalEntireTestConditional extends EntireTestConditional {
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
