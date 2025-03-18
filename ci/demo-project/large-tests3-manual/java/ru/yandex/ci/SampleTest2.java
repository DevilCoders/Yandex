package ru.yandex.ci;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

public class SampleTest2 {

    @ParameterizedTest
    @ValueSource(strings = {"s1", "s2", "s3", "s4", "s5"})
    void testAssertTrue(String step) {
        Assertions.assertFalse(step.isEmpty(), "must be true, " + step);
    }

    @Disabled
    @ParameterizedTest
    @ValueSource(strings = {"s1", "s2", "s3", "s4", "s5"})
    void testAssertFalse(String step) {
        Assertions.assertTrue(step.isEmpty(), "must be true, " + step);
    }

    @Disabled
    @SuppressWarnings("ConstantConditions")
    @Test
    void testFailure() {
        String none = null;
        Assertions.assertTrue(none.isEmpty(), "failure");
    }

}

