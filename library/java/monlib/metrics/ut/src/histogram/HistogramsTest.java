package ru.yandex.monlib.metrics.histogram;

import org.junit.Assert;
import org.junit.Test;


/**
 * @author Sergey Polovko
 */
public class HistogramsTest {

    @Test
    public void explicit() {
        HistogramCollector h = Histograms.explicit(0, 1, 2, 5, 10, 20);
        h.collect(-2);
        h.collect(-1);
        h.collect(0);
        h.collect(1);
        h.collect(20);
        h.collect(21);
        h.collect(1000);

        HistogramSnapshot snapshot = h.snapshot();
        Assert.assertEquals(7, snapshot.count());

        double[] expectedUpperBounds =  { 0, 1, 2, 5, 10, 20, Histograms.INF_BOUND };
        long[] expectedBucketValues = { 3, 1, 0, 0, 0,  1,  2 };

        for (int i = 0; i < snapshot.count(); i++) {
            Assert.assertEquals(expectedUpperBounds[i], snapshot.upperBound(i), Double.MIN_VALUE);
            Assert.assertEquals(expectedBucketValues[i], snapshot.value(i));
        }

        Assert.assertEquals("{0: 3, 1: 1, 2: 0, 5: 0, 10: 0, 20: 1, inf: 2}", snapshot.toString());

        HistogramSnapshot anotherSnapshot = h.snapshot();
        Assert.assertNotSame(anotherSnapshot, snapshot);
        Assert.assertEquals(anotherSnapshot, snapshot);

        Assert.assertEquals(
            "ExplicitCollector{" +
                "bounds=[0.0, 1.0, 2.0, 5.0, 10.0, 20.0, inf], " +
                "buckets=[3, 1, 0, 0, 0, 1, 2]" +
            "}",
            h.toString());
    }

    @Test
    public void exponential() {
        HistogramCollector h = Histograms.exponential(6, 2, 3);
        h.collect(-1);
        h.collect(0);
        h.collect(1);
        h.collect(3);
        h.collect(4);
        h.collect(5);
        h.collect(22);
        h.collect(23);
        h.collect(24);
        h.collect(50);
        h.collect(100);
        h.collect(1000);

        HistogramSnapshot snapshot = h.snapshot();
        Assert.assertEquals(6, snapshot.count());

        double[] expectedUpperBounds =  { 3, 6, 12, 24, 48, Histograms.INF_BOUND };
        long[] expectedBucketValues = { 4, 2, 0,  3,  0,  3 };

        for (int i = 0; i < snapshot.count(); i++) {
            Assert.assertEquals(expectedUpperBounds[i], snapshot.upperBound(i), Double.MIN_VALUE);
            Assert.assertEquals(expectedBucketValues[i], snapshot.value(i));
        }

        Assert.assertEquals("{3: 4, 6: 2, 12: 0, 24: 3, 48: 0, inf: 3}", snapshot.toString());

        HistogramSnapshot anotherSnapshot = h.snapshot();
        Assert.assertNotSame(anotherSnapshot, snapshot);
        Assert.assertEquals(anotherSnapshot, snapshot);

        Assert.assertEquals(
            "ExponentialCollector{" +
                "base=2.0, " +
                "scale=3.0, " +
                "buckets=[4, 2, 0, 3, 0, 3]" +
            "}",
            h.toString());
    }

    @Test
    public void linear() {
        HistogramCollector h = Histograms.linear(6, 5, 15);
        h.collect(-1);
        h.collect(0);
        h.collect(1);
        h.collect(4);
        h.collect(5);
        h.collect(6);
        h.collect(64);
        h.collect(65);
        h.collect(66);
        h.collect(100);
        h.collect(1000);

        HistogramSnapshot snapshot = h.snapshot();
        Assert.assertEquals(6, snapshot.count());

        double[] expectedUpperBounds =  { 5, 20, 35, 50, 65, Histograms.INF_BOUND };
        long[] expectedBucketValues = { 5, 1,  0,  0,  2,  3 };

        for (int i = 0; i < snapshot.count(); i++) {
            Assert.assertEquals(expectedUpperBounds[i], snapshot.upperBound(i), Double.MIN_VALUE);
            Assert.assertEquals(expectedBucketValues[i], snapshot.value(i));
        }

        Assert.assertEquals("{5: 5, 20: 1, 35: 0, 50: 0, 65: 2, inf: 3}", snapshot.toString());

        HistogramSnapshot anotherSnapshot = h.snapshot();
        Assert.assertNotSame(anotherSnapshot, snapshot);
        Assert.assertEquals(anotherSnapshot, snapshot);

        Assert.assertEquals(
            "LinearCollector{" +
                "startValue=5, " +
                "bucketWidth=15, " +
                "buckets=[5, 1, 0, 0, 2, 3]" +
            "}",
            h.toString());
    }

    @Test
    public void boundsEquals() {
        {
            HistogramSnapshot s1 = Histograms.exponential(6, 2, 3).snapshot();
            HistogramSnapshot s2 = Histograms.explicit(3, 6, 12, 24, 48).snapshot();
            Assert.assertTrue(s1.boundsEquals(s2));
            Assert.assertTrue(s2.boundsEquals(s1));
        }
        {
            HistogramSnapshot s1 = Histograms.linear(6, 5, 15).snapshot();
            HistogramSnapshot s2 = Histograms.explicit(5, 20, 35, 50, 65).snapshot();
            Assert.assertTrue(s1.boundsEquals(s2));
            Assert.assertTrue(s2.boundsEquals(s1));
        }
    }

    @Test
    public void bucketsEquals() {
        {
            HistogramSnapshot s1 = Histograms.exponential(6, 2)
                .collect(1)
                .collect(2)
                .collect(3)
                .collect(8)
                .collect(31)
                .collect(32)
                .collect(33)
                .snapshot();
            HistogramSnapshot s2 = Histograms.explicit(1, 2, 4, 8, 16)
                .collect(1)
                .collect(2)
                .collect(3)
                .collect(8)
                .collect(31)
                .collect(32)
                .collect(33)
                .snapshot();
            Assert.assertTrue(s1 + " != " + s2, s1.bucketsEquals(s2));
            Assert.assertTrue(s2 + " != " + s1, s2.bucketsEquals(s1));
        }
        {
            HistogramSnapshot s1 = Histograms.linear(6, 5, 15)
                .collect(4)
                .collect(5)
                .collect(6)
                .collect(64)
                .collect(65)
                .collect(66)
                .snapshot();
            HistogramSnapshot s2 = Histograms.explicit(5, 20, 35, 50, 65)
                .collect(4)
                .collect(5)
                .collect(6)
                .collect(64)
                .collect(65)
                .collect(66)
                .snapshot();
            Assert.assertTrue(s1 + " != " + s2, s1.bucketsEquals(s2));
            Assert.assertTrue(s2 + " != " + s1, s2.bucketsEquals(s1));
        }
    }
}
