package ru.yandex.monlib.metrics.meter;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

import javax.annotation.ParametersAreNonnullByDefault;

/**
 * @author Ivan Tsybulin
 */
@ParametersAreNonnullByDefault
public class TumblingMax {
    private volatile long prevWindow;
    private volatile long currWindow;
    private final long minValue;

    private static final AtomicLongFieldUpdater<TumblingMax> updater =
            AtomicLongFieldUpdater.newUpdater(TumblingMax.class, "currWindow");

    /**
     * @param minValue Max of an empty window. 0 or Long.MIN_VALUE depending on the context
     */
    public TumblingMax(long minValue) {
        this.minValue = minValue;
        prevWindow = currWindow = minValue;
    }

    public void accept(long value) {
        long old;
        do {
            old = updater.get(this);
            if (value <= old) {
                return;
            }
        } while (!updater.compareAndSet(this, old, value));
    }

    public long getMax() {
        // Window swap procedure
        // prev    curr    true max
        //  X       Y      max(X, Y)
        //  Y       Y          Y
        //  Y    minValue      Y
        final long left = prevWindow; // X or Y
        final long right = currWindow; // Y or minValue
        return left > right ? left : right; // X or Y for any order of execution
    }

    public void tick() {
        prevWindow = currWindow;
        currWindow = minValue;
    }
}
