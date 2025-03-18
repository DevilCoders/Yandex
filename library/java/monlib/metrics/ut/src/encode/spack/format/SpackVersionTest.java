package ru.yandex.monlib.metrics.encode.spack.format;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * @author Sergey Polovko
 */
public class SpackVersionTest {

    @Test
    public void byValue() {
        for (SpackVersion version : SpackVersion.ALL) {
            assertEquals(version, SpackVersion.byValue(version.getValue()));
        }

        assertNull(SpackVersion.byValue(0x0000));
    }

    @Test
    public void compare() {
        assertTrue(SpackVersion.v1_0.le(SpackVersion.v1_0));
        assertTrue(SpackVersion.v1_0.le(SpackVersion.v1_1));
        assertTrue(SpackVersion.v1_0.lt(SpackVersion.v1_1));
        assertTrue(SpackVersion.v1_0.lt(SpackVersion.v1_2));

        assertTrue(SpackVersion.v1_2.gt(SpackVersion.v1_0));
        assertTrue(SpackVersion.v1_2.gt(SpackVersion.v1_1));
        assertTrue(SpackVersion.v1_2.ge(SpackVersion.v1_1));
        assertTrue(SpackVersion.v1_2.ge(SpackVersion.v1_2));

        assertFalse(SpackVersion.v1_2.lt(SpackVersion.v1_1));
        assertFalse(SpackVersion.v1_0.gt(SpackVersion.v1_1));
    }
}
