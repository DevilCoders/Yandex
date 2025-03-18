package ru.yandex.monlib.metrics.meter;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;


/**
 * @author Sergey Polovko
 */
public abstract class TickMixin {

    private static final AtomicLongFieldUpdater<TickMixin> lastTickUpdater =
        AtomicLongFieldUpdater.newUpdater(TickMixin.class, "lastTick");

    private final long tickIntervalNanos;
    private volatile long lastTick;

    protected TickMixin(long tickIntervalNanos) {
        this.tickIntervalNanos = tickIntervalNanos;
        final long tick = System.nanoTime();
        lastTick = tick - tick % tickIntervalNanos;
    }

    protected abstract void onTick();

    protected void tickIfNecessary() {
        final long oldTick = lastTickUpdater.get(this);
        final long newTick = System.nanoTime();
        final long age = newTick - oldTick;
        if (age > tickIntervalNanos) {
            final long newIntervalStartTick = newTick - age % tickIntervalNanos;
            if (lastTickUpdater.compareAndSet(this, oldTick, newIntervalStartTick)) {
                final long requiredTicks = age / tickIntervalNanos;
                for (long i = 0; i < requiredTicks; i++) {
                    onTick();
                }
            }
        }
    }
}
