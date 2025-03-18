package ru.yandex.ci.core.poller;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.IntFunction;
import java.util.function.Predicate;

import com.google.common.base.Preconditions;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class Poller<T> {
    private static final Logger log = LogManager.getLogger();
    private static final long ONE_SECOND_MILLIS = TimeUnit.SECONDS.toMillis(1);

    private final Callable<T> action;
    private final Predicate<T> canStopWhen;
    private final PollerOptions pollerOptions;
    private final Sleeper sleeper;

    public Poller(PollerBuilder<T> builder) {
        this.action = builder.action;
        this.canStopWhen = builder.canStopWhen;
        this.pollerOptions = builder.pollerOptionsBuilder.build();
        this.sleeper = builder.sleeper;
    }

    public static <T> PollerBuilder<T> poll(Callable<T> action) {
        return new PollerBuilder<T>().poll(action);
    }

    public static PollerBuilder<Boolean> pollBoolean(Callable<Boolean> action) {
        return poll(action).canStopWhen(b -> b);
    }

    public static <T> PollerBuilder<T> pollNullable(Callable<T> action) {
        return poll(action).canStopWhen(Objects::nonNull);
    }

    public static <T> PollerBuilder<Optional<T>> pollOptional(Callable<Optional<T>> action) {
        return poll(action).canStopWhen(Optional::isPresent);
    }

    public static <T> PollerBuilder<T> builder() {
        return new PollerBuilder<>();
    }

    public T run() throws TimeoutException, InterruptedException {
        long startTimeMillis = System.currentTimeMillis();
        int retryCount = 0;
        int currentRetryOnException = 0;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                T state = action.call();

                if (canStopWhen.test(state)) {
                    return state;
                }
                currentRetryOnException = 0;
            } catch (InterruptedException e) {
                log.error("Poll interrupted", e);
                throw e;
            } catch (Throwable e) {
                if (!pollerOptions.getIgnoringExceptions().contains(e.getClass())) {
                    log.error("Exception while polling: ", e);

                    boolean shouldRetry = pollerOptions.getOnThrow() == null || pollerOptions.getOnThrow().test(e);

                    if (!shouldRetry) {
                        throwRuntimeException(e);
                    }

                    if (++currentRetryOnException >= pollerOptions.getRetryOnExceptionCount()) {
                        throwRuntimeException(e);
                    }

                    log.info("Retrying after exception. Retry number: " + currentRetryOnException);
                } else {
                    log.warn("Exception while polling: ", e);
                }
            }

            long elapsedTimeMillis = System.currentTimeMillis() - startTimeMillis;
            if (elapsedTimeMillis >= pollerOptions.getTimeoutMillis()) {
                throw new TimeoutException(String.format("Timeout after %d millis", elapsedTimeMillis));
            }

            long sleepIntervalMillis = pollerOptions.getIntervalTimeUnit()
                    .toMillis(pollerOptions.getInterval().apply(++retryCount));

            if (elapsedTimeMillis + sleepIntervalMillis > pollerOptions.getTimeoutMillis()) {
                long newSleepIntervalMillis = pollerOptions.getTimeoutMillis() - elapsedTimeMillis;
                if (!pollerOptions.getAllowIntervalLessThenOneSecond() && newSleepIntervalMillis < ONE_SECOND_MILLIS) {
                    newSleepIntervalMillis = ONE_SECOND_MILLIS;
                }
                log.info("Reducing sleep interval {} millis to {} millis to fit timeout {} millis" +
                                " (allowIntervalLessThenOneSecond {})",
                        sleepIntervalMillis, newSleepIntervalMillis, pollerOptions.getTimeoutMillis(),
                        pollerOptions.getAllowIntervalLessThenOneSecond());
                sleepIntervalMillis = newSleepIntervalMillis;
            }

            Preconditions.checkState(
                    pollerOptions.getAllowIntervalLessThenOneSecond() ||
                            sleepIntervalMillis >= TimeUnit.SECONDS.toMillis(1),
                    "Poll interval can not be less then 1 second"
            );

            sleeper.sleep(Duration.of(sleepIntervalMillis, ChronoUnit.MILLIS));
        }

        throw new InterruptedException();
    }

    private void throwRuntimeException(Throwable e) {
        if (e instanceof RuntimeException) {
            throw (RuntimeException) e;
        }

        throw new RuntimeException("Exception while polling: ", e);
    }

    public static class PollerBuilder<T> {
        private Callable<T> action;
        private Predicate<T> canStopWhen = t -> true;
        private PollerOptions.PollerOptionsBuilder pollerOptionsBuilder = PollerOptions.builder();
        private Sleeper sleeper = Sleeper.DEFAULT;

        public PollerBuilder<T> poll(Callable<T> action) {
            this.action = action;
            return this;
        }

        public PollerBuilder<T> canStopWhen(Predicate<T> canStopWhen) {
            this.canStopWhen = canStopWhen;
            return this;
        }

        public PollerBuilder<T> timeout(long timeout, TimeUnit unit) {
            pollerOptionsBuilder.timeout(timeout, unit);
            return this;
        }

        public PollerBuilder<T> timeout(Duration duration) {
            pollerOptionsBuilder.timeout(duration.toMillis(), TimeUnit.MILLISECONDS);
            return this;
        }

        public PollerBuilder<T> interval(IntFunction<Long> interval, TimeUnit unit) {
            pollerOptionsBuilder.interval(interval, unit);
            return this;
        }

        public PollerBuilder<T> interval(Duration duration) {
            pollerOptionsBuilder.interval(duration.toMillis(), TimeUnit.MILLISECONDS);
            return this;
        }

        public PollerBuilder<T> interval(long interval, TimeUnit unit) {
            pollerOptionsBuilder.interval(interval, unit);
            return this;
        }

        public PollerBuilder<T> allowIntervalLessThenOneSecond(boolean allow) {
            pollerOptionsBuilder.allowIntervalLessThenOneSecond(allow);
            return this;
        }

        public PollerBuilder<T> ignoring(Class<? extends Throwable> throwable) {
            pollerOptionsBuilder.ignoring(throwable);
            return this;
        }

        public PollerBuilder<T> retryOnExceptionCount(int retryOnExceptionCount) {
            pollerOptionsBuilder.retryOnExceptionCount(retryOnExceptionCount);
            return this;
        }

        /**
         * Функция обработки исключения.
         *
         * @param onThrow - функция обработки исключения. Должна возвращать:
         *                true, если после обработки необходимо поретраить исключение
         *                false, если необходимо выбросить исключение и остановить поллинг
         * @return текущий класс
         */
        public PollerBuilder<T> onThrow(Predicate<Throwable> onThrow) {
            pollerOptionsBuilder.onThrow(onThrow);
            return this;
        }

        public PollerBuilder<T> pollerOptions(PollerOptions.PollerOptionsBuilder pollerOptionsBuilder) {
            this.pollerOptionsBuilder = pollerOptionsBuilder;
            return this;
        }

        public PollerBuilder<T> sleeper(Sleeper sleeper) {
            this.sleeper = sleeper;
            return this;
        }

        public Poller<T> build() {
            return new Poller<>(this);
        }

        public T run() throws TimeoutException, InterruptedException {
            return build().run();
        }
    }

    @FunctionalInterface
    public interface Sleeper {
        Sleeper DEFAULT = duration -> Thread.sleep(duration.toMillis());

        void sleep(Duration duration) throws InterruptedException;
    }
}
