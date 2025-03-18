package ru.yandex.ci.util;

import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.function.Supplier;

import lombok.extern.slf4j.Slf4j;

@Slf4j
public class Retryable {
    private static boolean disable = false;

    public static final int START_TIMEOUT_SECONDS = 1;
    public static final int MAX_TIMEOUT_SECONDS = 32;

    private Retryable() {

    }

    public static void disable() {
        disable = true;
    }

    public static void retryUntilInterruptedOrSucceeded(Runnable action, Consumer<Throwable> onRetry) {
        retryUntilInterruptedOrSucceeded(action, onRetry, false, START_TIMEOUT_SECONDS, MAX_TIMEOUT_SECONDS);
    }

    public static void retryUntilInterruptedOrSucceeded(
            Runnable action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn
    ) {
        retryUntilInterruptedOrSucceeded(
                action, onRetry, reduceLogLevelToWarn, START_TIMEOUT_SECONDS, MAX_TIMEOUT_SECONDS
        );
    }

    public static void retryUntilInterruptedOrSucceeded(
            Runnable action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn, int startTimeout, int maxTimeout
    ) {
        retryUntilInterruptedOrSucceeded(
                () -> {
                    action.run();
                    return null;
                },
                onRetry, reduceLogLevelToWarn, startTimeout, maxTimeout
        );
    }

    public static <T> T retryUntilInterruptedOrSucceeded(Supplier<T> action, Consumer<Throwable> onRetry) {
        return retryUntilInterruptedOrSucceeded(action, onRetry, false, START_TIMEOUT_SECONDS, MAX_TIMEOUT_SECONDS);
    }

    public static <T> T retryUntilInterruptedOrSucceeded(
            Supplier<T> action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn
    ) {
        return retryUntilInterruptedOrSucceeded(
                action, onRetry, reduceLogLevelToWarn, START_TIMEOUT_SECONDS, MAX_TIMEOUT_SECONDS
        );
    }

    public static <T> T retryUntilInterruptedOrSucceeded(
            Supplier<T> action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn, int startTimeout,
            int maxTimeout
    ) {
        return retry(action, onRetry, reduceLogLevelToWarn, startTimeout, maxTimeout, 0);
    }

    public static void retry(
            Runnable action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn, int startTimeout,
            int maxTimeout, int maxRetries
    ) {
        retry(
                () -> {
                    action.run();
                    return null;
                },
                onRetry, reduceLogLevelToWarn, startTimeout, maxTimeout, maxRetries
        );
    }

    public static <T> T retry(
            Supplier<T> action, Consumer<Throwable> onRetry, boolean reduceLogLevelToWarn, int startTimeout,
            int maxTimeout, int maxRetries
    ) {
        var retryTimeoutSeconds = startTimeout;

        var retry = 0;
        while (!Thread.interrupted()) {
            try {
                return action.get();
            } catch (StorageInterruptedException e) {
                throw e;
            } catch (Throwable e) {
                if (e.getCause() != null && e.getCause() instanceof InterruptedException) {
                    log.info("Action was interrupted by thread interruption", e.getCause());
                    throw new StorageInterruptedException(e);
                }

                onRetry.accept(e);

                if (e instanceof Exception) {
                    if (reduceLogLevelToWarn) {
                        log.warn("Failed to run action", e);
                    } else {
                        log.error("Failed to run action", e);
                    }
                } else {
                    log.error("Internal java error. Failed to run action", e);
                }

                retry++;
                if (disable || (maxRetries > 0 && retry > maxRetries)) {
                    throw e;
                }

                try {
                    Thread.sleep(TimeUnit.SECONDS.toMillis(retryTimeoutSeconds));
                    retryTimeoutSeconds = Math.min(retryTimeoutSeconds * 2, maxTimeout);
                } catch (InterruptedException interruptedException) {
                    log.info("Action was interrupted by thread interruption", e);
                    throw new StorageInterruptedException(interruptedException);
                }
            }
        }

        throw new StorageInterruptedException();
    }
}
