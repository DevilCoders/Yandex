package ru.yandex.ci.storage.core.utils;

import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.util.StorageInterruptedException;

@Slf4j
public class OptionalWaiter {

    private OptionalWaiter() {

    }

    public static <T> Optional<T> waitForNoneEmpty(
            Supplier<Optional<T>> supplier, Supplier<String> printer, int periodSeconds, int timeout
    ) {
        var stopwatch = Stopwatch.createStarted();

        while (true) {
            var result = supplier.get();
            if (result.isPresent()) {
                stopwatch.stop();

                if (stopwatch.elapsed(TimeUnit.SECONDS) > 0) {
                    log.info("Wait for {} completed after {}", printer.get(), stopwatch);
                }

                return result;
            }

            if (stopwatch.elapsed(TimeUnit.SECONDS) > 30) {
                log.warn("Still waiting {} for {}", printer.get(), stopwatch);
            }

            if (stopwatch.elapsed(TimeUnit.SECONDS) > timeout) {
                log.warn("Timeout waiting {} for {}", printer.get(), stopwatch);
                return Optional.empty();
            }

            try {
                //noinspection BusyWait
                Thread.sleep(periodSeconds);
            } catch (InterruptedException e) {
                throw new StorageInterruptedException(e);
            }
        }
    }

}
