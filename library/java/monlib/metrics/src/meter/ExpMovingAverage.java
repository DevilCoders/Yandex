package ru.yandex.monlib.metrics.meter;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLongFieldUpdater;
import java.util.concurrent.atomic.LongAdder;

import static java.lang.Double.doubleToRawLongBits;
import static java.lang.Double.longBitsToDouble;
import static java.lang.Math.exp;

/**
 * An exponentially-weighted moving average (EWMA).
 *
 * @see <a href="http://www.teamquest.com/pdfs/whitepaper/ldavg1.pdf">UNIX Load Average Part 1: How
 *      It Works</a>
 * @see <a href="http://www.teamquest.com/pdfs/whitepaper/ldavg2.pdf">UNIX Load Average Part 2: Not
 *      Your Average Average</a>
 * @see <a href="http://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average">EMA</a>
 *
 * @author Sergey Polovko
 */
public class ExpMovingAverage {
    public static final int INTERVAL_SECONDS = 5;

    private static final double SECONDS_PER_MINUTE = 60.0;
    private static final int ONE_MINUTE = 1;
    private static final int FIVE_MINUTES = 5;
    private static final int FIFTEEN_MINUTES = 15;
    private static final double M1_ALPHA = 1 - exp(-INTERVAL_SECONDS / SECONDS_PER_MINUTE / ONE_MINUTE);
    private static final double M5_ALPHA = 1 - exp(-INTERVAL_SECONDS / SECONDS_PER_MINUTE / FIVE_MINUTES);
    private static final double M15_ALPHA = 1 - exp(-INTERVAL_SECONDS / SECONDS_PER_MINUTE / FIFTEEN_MINUTES);

    private volatile boolean initialized = false;
    private volatile long rate = 0;
    private static final AtomicLongFieldUpdater<ExpMovingAverage> updater =
        AtomicLongFieldUpdater.newUpdater(ExpMovingAverage.class, "rate");

    private final LongAdder uncounted = new LongAdder();
    private final double alpha, reciprocalInterval;
    private final long interval;

    /**
     * Creates a new EWMA which is equivalent to the UNIX one minute load average and which expects
     * to be ticked every 5 seconds.
     *
     * @return a one-minute EWMA
     */
    public static ExpMovingAverage oneMinute() {
        return new ExpMovingAverage(M1_ALPHA, INTERVAL_SECONDS, TimeUnit.SECONDS);
    }

    /**
     * Creates a new EWMA which is equivalent to the UNIX five minute load average and which expects
     * to be ticked every 5 seconds.
     *
     * @return a five-minute EWMA
     */
    public static ExpMovingAverage fiveMinutes() {
        return new ExpMovingAverage(M5_ALPHA, INTERVAL_SECONDS, TimeUnit.SECONDS);
    }

    /**
     * Creates a new EWMA which is equivalent to the UNIX fifteen minute load average and which
     * expects to be ticked every 5 seconds.
     *
     * @return a fifteen-minute EWMA
     */
    public static ExpMovingAverage fifteenMinutes() {
        return new ExpMovingAverage(M15_ALPHA, INTERVAL_SECONDS, TimeUnit.SECONDS);
    }

    /**
     * Create a new EWMA with a specific smoothing constant.
     *
     * @param alpha        the smoothing constant, should be equal to 1 - exp(-tickInterval / smoothingInterval)
     * @param tickInterval the expected tick interval
     * @param intervalUnit the time unit of the tick interval
     */
    public ExpMovingAverage(double alpha, long tickInterval, TimeUnit intervalUnit) {
        this.interval = intervalUnit.toNanos(tickInterval);
        this.reciprocalInterval = 1d / interval;
        this.alpha = alpha;
    }

    /**
     * How often should this EWMA be ticked.
     *
     * @return tickInterval in nanoseconds
     */
    public long getTickIntervalNanos() {
        return interval;
    }

    /**
     * Update the moving average with a new value.
     *
     * @param n the new value
     */
    public void update(long n) {
        uncounted.add(n);
    }

    /**
     * Increment counter
     */
    public void inc() {
        uncounted.increment();
    }

    /**
     * Reset EWMA into initial state.
     */
    public void reset() {
        uncounted.reset();
        rate = 0;
        initialized = false;
    }

    /**
     * Mark the passage of time and decay the current rate accordingly.
     */
    public void tick() {
        final long count = uncounted.sumThenReset();
        final double instantRate = count * reciprocalInterval;
        if (initialized) {
            long old;
            long newValue;
            do {
                old = rate;
                double oldDoubleValue = longBitsToDouble(old);
                newValue = doubleToRawLongBits(alpha * (instantRate - oldDoubleValue) + oldDoubleValue);
            } while (!updater.compareAndSet(this, old, newValue));
        } else {
            rate = doubleToRawLongBits(instantRate);
            initialized = true;
        }
    }

    /**
     * Returns the rate in the given units of time.
     *
     * @param rateUnit the unit of time
     * @return the rate
     */
    public double getRate(TimeUnit rateUnit) {
        return longBitsToDouble(rate) * (double) rateUnit.toNanos(1);
    }

    public void combine(ExpMovingAverage exp) {
        if (Double.compare(alpha, exp.alpha) != 0) {
            throw new IllegalArgumentException("Not able combine ExpMovingAverage with different alpha");
        }

        initialized = true;
        uncounted.add(exp.uncounted.sum());
        long old;
        long newValue;
        do {
            old = rate;
            newValue = doubleToRawLongBits(longBitsToDouble(old) + longBitsToDouble(exp.rate));
        } while (!updater.compareAndSet(this, old, newValue));
    }
}
