package ru.yandex.ci.flow.utils;

import java.time.Duration;
import java.time.Instant;

public class InstantUtils {
    private InstantUtils() {

    }

    public static long calculateRemainingTimeMillis(
        Instant startTimestamp, Instant nowTimestamp, long totalTimeMillis
    ) {
        long remainingTime = totalTimeMillis - Duration.between(startTimestamp, nowTimestamp).toMillis();
        return (remainingTime < 0) ? 0 : remainingTime;
    }
}
