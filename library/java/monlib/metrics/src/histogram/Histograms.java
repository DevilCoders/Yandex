package ru.yandex.monlib.metrics.histogram;

import java.util.Objects;

import javax.annotation.Nonnull;


/**
 * @author Sergey Polovko
 */
public final class Histograms {
    private Histograms() {}

    public static final int MAX_BUCKETS_COUNT = 51; // may be the subject of change

    public static final double INF_BOUND = Double.MAX_VALUE;

    /**
     * <p>Creates histogram collector for a set of buckets with arbitrary bounds.</p>
     *
     * <p>Defines {@code bounds.length  + 1} buckets with these boundaries for bucket i:</p>
     * <ul>
     *     <li>Upper bound (0 <= i < N-1): {@code bounds[i]}</li>
     *     <li>Lower bound (1 <= i < N):   {@code bounds[i - 1]}</li>
     * </ul>
     *
     * <p>For example, if the list of boundaries is:</p>
     * <pre>0, 1, 2, 5, 10, 20</pre>
     *
     * <p>then there are five finite buckets with the following ranges:</p>
     * <pre>(-INF, 0], (0, 1], (1, 2], (2, 5], (5, 10], (10, 20], (20, +INF)</pre>
     *
     * @param bounds array of upper bounds for buckets. Values must be sorted.
     */
    public static HistogramCollector explicit(@Nonnull double... bounds) {
        Objects.requireNonNull(bounds, "given array of bounds is null");
        checkBucketsCount(bounds.length, 1);
        if (!Arrays.isSorted(bounds)) {
            throw new IllegalArgumentException("given array of bounds is not sorted");
        }
        return new ExplicitCollector(bounds);
    }

    /**
     * Shortcut for {@link Histograms#exponential(int, double, double)} with {@code scale=1.0}.
     */
    public static HistogramCollector exponential(int bucketsCount, double base) {
        return exponential(bucketsCount, base, 1.0);
    }

    /**
     * <p>Creates histogram collector for a sequence of buckets that have a width proportional to
     * the value of the lower bound.</p>
     *
     * <p>Defines {@code bucketsCount} buckets with these boundaries for bucket i:</p>
     * <ul>
     *    <li>Upper bound (0 <= i < N-1):  {@code scale * (base ^ i)}</li>
     *    <li>Lower bound (1 <= i < N):    {@code scale * (base ^ (i - 1))}</li>
     * </ul>
     *
     * <p>For example, if {@code bucketsCount=6}, {@code base=2}, and {@code scale=3},
     * then the bucket ranges are as follows:</p>
     *
     * <pre>(-INF, 3], (3, 6], (6, 12], (12, 24], (24, 48], (48, +INF)</pre>
     *
     * @param bucketsCount the total number of buckets. The value must be >= 2.
     * @param base         the exponential growth factor for the buckets width.
     *                     The value must be >= 1.0.
     * @param scale        the linear scale for the buckets. The value must be >= 1.0.
     */
    public static HistogramCollector exponential(int bucketsCount, double base, double scale) {
        checkBucketsCount(bucketsCount, 2);
        if (base < 1.0) {
            throw new IllegalArgumentException("base must be >= 1.0, got: " + base);
        }
        if (scale < 1.0) {
            throw new IllegalArgumentException("scale must be >= 1.0, scale: " + scale);
        }
        return new ExponentialCollector(bucketsCount, base, scale);
    }

    /**
     * <p>Creates histogram collector for a sequence of buckets that all have the same
     * width (except overflow and underflow).</p>
     *
     * <p>Defines {@code bucketsCount} buckets with these boundaries for bucket i:</p>
     * <ul>
     *    <li>Upper bound (0 <= i < N-1):  {@code startValue + bucketWidth * i}</li>
     *    <li>Lower bound (1 <= i < N):    {@code startValue + bucketWidth * (i - 1)}</li>
     * </ul>
     *
     * <p>For example, if {@code bucketsCount=6}, {@code startValue=5}, and {@code bucketWidth=15},
     * then the bucket ranges are as follows:</p>
     *
     * <pre>(-INF, 5], (5, 20], (20, 35], (35, 50], (50, 65], (65, +INF)</pre>
     *
     * @param bucketsCount the total number of buckets. The value must be >= 2.
     * @param startValue   the upper boundary of the first bucket.
     * @param bucketWidth  the difference between the upper and lower bounds for each bucket.
     *                     The value must be >= 1.
     */
    public static HistogramCollector linear(int bucketsCount, long startValue, long bucketWidth) {
        checkBucketsCount(bucketsCount, 2);
        if (bucketWidth < 1) {
            throw new IllegalArgumentException("bucketWidth must be >= 1, got: " + bucketWidth);
        }
        return new LinearCollector(bucketsCount, startValue, bucketWidth);
    }

    private static void checkBucketsCount(int bucketsCount, int min) {
        if (bucketsCount < min) {
            throw new IllegalArgumentException("bucketsCount must be > " + min + ", got: " + bucketsCount);
        }
        if (bucketsCount > MAX_BUCKETS_COUNT) {
            throw new IllegalArgumentException("bucketsCount must <= " + MAX_BUCKETS_COUNT + ", got: " + bucketsCount);
        }
    }
}
