package ru.yandex.monlib.metrics.meter;

import java.util.concurrent.TimeUnit;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * @author Sergey Polovko
 */
public class ExpMovingAverageTest {

    private static final double EPS = 0.000001;

    @Test
    public void oneMinute() throws Exception {
        ExpMovingAverage ema = ExpMovingAverage.oneMinute();
        ema.update(3);
        ema.tick();

        double[] expectedValues = new double[] {
            0.6,
            0.22072766,
            0.08120117,
            0.02987224,
            0.01098938,
            0.00404277,
            0.00148725,
            0.00054713,
            0.00020128,
            0.00007405,
            0.00002724,
            0.00001002,
            0.00000369,
            0.00000136,
            0.00000050,
            0.00000018,
        };

        for (double expectedValue : expectedValues) {
            assertEquals(ema.getRate(TimeUnit.SECONDS), expectedValue, EPS);
            elapseOneMinute(ema);
        }
    }

    @Test
    public void combine() {
        ExpMovingAverage one = ExpMovingAverage.oneMinute();
        one.update(3);
        one.tick();

        assertEquals(one.getRate(TimeUnit.SECONDS), 0.6, EPS);

        ExpMovingAverage two = ExpMovingAverage.oneMinute();
        two.update(6);
        two.tick();
        assertEquals(two.getRate(TimeUnit.SECONDS), 1.2, EPS);

        ExpMovingAverage target = ExpMovingAverage.oneMinute();

        target.combine(one);
        assertEquals(0.6, target.getRate(TimeUnit.SECONDS), EPS);

        target.combine(two);
        assertEquals(0.6 + 1.2, target.getRate(TimeUnit.SECONDS), EPS);

        elapseOneMinute(one);
        assertEquals(0.22072766, one.getRate(TimeUnit.SECONDS), EPS);

        elapseOneMinute(two);
        assertEquals(0.44145532, two.getRate(TimeUnit.SECONDS), EPS);

        elapseOneMinute(target);
        assertEquals(0.22072766 + 0.44145532, target.getRate(TimeUnit.SECONDS),  EPS);
    }

    @Test(expected = IllegalArgumentException.class)
    public void combineDiffAlpha() {
        ExpMovingAverage five = ExpMovingAverage.fiveMinutes();
        five.update(3);
        five.tick();

        ExpMovingAverage one = ExpMovingAverage.oneMinute();
        one.combine(five);
    }

    @Test
    public void fiveMinutes() throws Exception {
        ExpMovingAverage ema = ExpMovingAverage.fiveMinutes();
        ema.update(3);
        ema.tick();

        double[] expectedValues = new double[] {
            0.6,
            0.49123845,
            0.40219203,
            0.32928698,
            0.26959738,
            0.22072766,
            0.18071653,
            0.14795818,
            0.12113791,
            0.09917933,
            0.08120117,
            0.06648190,
            0.05443077,
            0.04456415,
            0.03648604,
            0.02987224,
        };

        for (double expectedValue : expectedValues) {
            assertEquals(ema.getRate(TimeUnit.SECONDS), expectedValue, EPS);
            elapseOneMinute(ema);
        }
    }

    @Test
    public void fifteenMinutes() throws Exception {
        ExpMovingAverage ema = ExpMovingAverage.fifteenMinutes();
        ema.update(3);
        ema.tick();

        double[] expectedValues = new double[] {
            0.6,
            0.56130419,
            0.52510399,
            0.49123845,
            0.45955700,
            0.42991879,
            0.40219203,
            0.37625345,
            0.35198773,
            0.32928698,
            0.30805027,
            0.28818318,
            0.26959738,
            0.25221023,
            0.23594443,
            0.22072766,
        };

        for (double expectedValue : expectedValues) {
            assertEquals(ema.getRate(TimeUnit.SECONDS), expectedValue, EPS);
            elapseOneMinute(ema);
        }
    }

    private static void elapseOneMinute(ExpMovingAverage ema) {
        for (int i = 0; i < 12; i++) {
            ema.tick(); // each tick has 5 seconds interval
        }
    }
}
