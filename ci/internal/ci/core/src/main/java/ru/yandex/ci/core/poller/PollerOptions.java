package ru.yandex.ci.core.poller;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.function.IntFunction;
import java.util.function.Predicate;

import com.google.common.base.Preconditions;

public class PollerOptions {
    private final long timeoutMillis;
    private final IntFunction<Long> interval;
    private final TimeUnit intervalTimeUnit;
    private final List<Class<? extends Throwable>> ignoringExceptions;
    private final int retryOnExceptionCount;
    private final Predicate<Throwable> onThrow;
    private final boolean allowIntervalLessThenOneSecond;

    public PollerOptions(PollerOptionsBuilder builder) {
        this.timeoutMillis = builder.timeoutMillis;
        this.intervalTimeUnit = builder.intervalTimeUnit;
        this.interval = builder.interval;
        this.ignoringExceptions = Collections.unmodifiableList(builder.ignoringExceptions);
        this.retryOnExceptionCount = builder.retryOnExceptionCount;
        this.onThrow = builder.onThrow;
        this.allowIntervalLessThenOneSecond = builder.allowIntervalLessThenOneSecond;
    }

    public static PollerOptionsBuilder builder() {
        return new PollerOptionsBuilder();
    }

    public long getTimeoutMillis() {
        return timeoutMillis;
    }

    public IntFunction<Long> getInterval() {
        return interval;
    }

    public TimeUnit getIntervalTimeUnit() {
        return intervalTimeUnit;
    }

    public boolean getAllowIntervalLessThenOneSecond() {
        return allowIntervalLessThenOneSecond;
    }

    public List<Class<? extends Throwable>> getIgnoringExceptions() {
        return ignoringExceptions;
    }

    public int getRetryOnExceptionCount() {
        return retryOnExceptionCount;
    }

    public Predicate<Throwable> getOnThrow() {
        return onThrow;
    }

    public static class PollerOptionsBuilder {
        private long timeoutMillis = TimeUnit.DAYS.toMillis(1);
        private IntFunction<Long> interval = t -> 10L;
        private TimeUnit intervalTimeUnit = TimeUnit.SECONDS;
        private final List<Class<? extends Throwable>> ignoringExceptions = new ArrayList<>();
        private int retryOnExceptionCount = 10;
        private Predicate<Throwable> onThrow = t -> true;
        private boolean allowIntervalLessThenOneSecond = false;

        public PollerOptions.PollerOptionsBuilder timeout(long timeout, TimeUnit unit) {
            Preconditions.checkArgument(timeout > 0);
            this.timeoutMillis = unit.toMillis(timeout);
            return this;
        }

        public PollerOptions.PollerOptionsBuilder interval(IntFunction<Long> interval, TimeUnit unit) {
            this.interval = interval;
            this.intervalTimeUnit = unit;
            return this;
        }

        public PollerOptions.PollerOptionsBuilder interval(long interval, TimeUnit unit) {
            this.interval = t -> interval;
            this.intervalTimeUnit = unit;
            return this;
        }

        public PollerOptions.PollerOptionsBuilder allowIntervalLessThenOneSecond(boolean allow) {
            this.allowIntervalLessThenOneSecond = allow;
            return this;
        }

        public PollerOptions.PollerOptionsBuilder ignoring(Class<? extends Throwable> throwable) {
            ignoringExceptions.add(throwable);
            return this;
        }

        public PollerOptions.PollerOptionsBuilder retryOnExceptionCount(int retryOnExceptionCount) {
            this.retryOnExceptionCount = retryOnExceptionCount;
            return this;
        }

        /**
         * Функция обработки исключения.
         *
         * @param onThrow - функция обработки исключения. Должна возвращать:
         *                true, если после обработки необходимо выбросить исключение
         *                false, если необходимо проигнорировать исключение и продолжить работу
         * @return текущий класс
         */
        public PollerOptions.PollerOptionsBuilder onThrow(Predicate<Throwable> onThrow) {
            this.onThrow = onThrow;
            return this;
        }

        public PollerOptions build() {
            return new PollerOptions(this);
        }
    }
}
