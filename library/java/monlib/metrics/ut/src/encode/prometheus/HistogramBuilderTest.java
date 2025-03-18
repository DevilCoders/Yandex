package ru.yandex.monlib.metrics.encode.prometheus;

import java.util.Collections;

import org.junit.Assert;
import org.junit.Test;

import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;


/**
 * @author Sergey Polovko
 */
public class HistogramBuilderTest {

    @Test
    public void empty() {
        HistogramBuilder hb = new HistogramBuilder();
        Assert.assertTrue(hb.isEmpty());
        Assert.assertEquals("", hb.getName());
        Assert.assertEquals(Collections.emptyMap(), hb.getLabels());
        Assert.assertEquals(0, hb.getTsMillis());

        try {
            hb.toSnapshot();
            Assert.fail("expected exception not thrown");
        } catch (Exception e) {
            Assert.assertEquals("histogram cannot be empty", e.getMessage());
        }
    }

    @Test
    public void nonEmpty() {
        HistogramBuilder hb = new HistogramBuilder();
        hb.setName("http_request_duration_seconds");
        hb.setLabels(Collections.singletonMap("path", "/"));
        hb.setTsMillis(1234567890);
        hb.addBucket(0.05, 24054);
        hb.addBucket(0.1, 33444);
        hb.addBucket(0.2, 100392);
        hb.addBucket(0.5, 129389);
        hb.addBucket(1, 133988);
        hb.addBucket(Double.POSITIVE_INFINITY, 144320);

        Assert.assertFalse(hb.isEmpty());
        Assert.assertEquals("http_request_duration_seconds", hb.getName());
        Assert.assertEquals(Collections.singletonMap("path", "/"), hb.getLabels());
        Assert.assertEquals(1234567890, hb.getTsMillis());

        HistogramSnapshot h = hb.toSnapshot();
        Assert.assertEquals(
            "{0.05: 24054, 0.1: 9390, 0.2: 66948, 0.5: 28997, 1: 4599, inf: 10332}",
            h.toString());

        hb.clear();
        Assert.assertTrue(hb.isEmpty());
        Assert.assertEquals("", hb.getName());
        Assert.assertEquals(Collections.emptyMap(), hb.getLabels());
        Assert.assertEquals(0, hb.getTsMillis());
    }
}
