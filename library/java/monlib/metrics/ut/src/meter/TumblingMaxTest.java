package ru.yandex.monlib.metrics.meter;

import java.util.Objects;

import javax.annotation.ParametersAreNonnullByDefault;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

/**
 * @author Ivan Tsybulin
 */
@ParametersAreNonnullByDefault
public class TumblingMaxTest {
    private TumblingMax tumblingMax;

    @Before
    public void setUp() {
        tumblingMax = new TumblingMax(42);
    }

    @Test
    public void empty() {
        for (int i = 0; i < 100; i++) {
            Assert.assertEquals(42, tumblingMax.getMax());
            if (Objects.hashCode(i) % 3 == 0) {
                tumblingMax.tick();
            }
        }
    }

    @Test
    public void spikeEvicted() {
        tumblingMax.accept(100500);
        Assert.assertEquals(100500, tumblingMax.getMax());
        tumblingMax.tick();
        Assert.assertEquals(100500, tumblingMax.getMax());
        tumblingMax.tick();
        Assert.assertEquals(42, tumblingMax.getMax());
    }

    @Test
    public void secondWindowMaxPreserved() {
        tumblingMax.accept(100500);
        Assert.assertEquals(100500, tumblingMax.getMax());
        tumblingMax.tick();
        tumblingMax.accept(50);
        tumblingMax.accept(6);
        Assert.assertEquals(100500, tumblingMax.getMax());
        tumblingMax.accept(40);
        tumblingMax.tick();
        Assert.assertEquals(50, tumblingMax.getMax());
    }
}
