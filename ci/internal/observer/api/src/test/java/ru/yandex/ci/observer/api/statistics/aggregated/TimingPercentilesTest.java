package ru.yandex.ci.observer.api.statistics.aggregated;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class TimingPercentilesTest {
    private static final Set<Integer> LEVELS = Set.of(1, 42, 50, 80, 90, 95, 99, 100);

    @Test
    void computeLongEmpty() {
        testComputeLong(
                LEVELS,
                List.of(),
                Map.of(1, 0L, 42, 0L, 50, 0L, 80, 0L, 90, 0L, 95, 0L, 99, 0L, 100, 0L)
        );
    }

    @Test
    void computeLong() {
        testComputeLong(
                LEVELS,
                List.of(1L, 2L, 3L),
                Map.of(1, 1L, 42, 2L, 50, 2L, 80, 3L, 90, 3L, 95, 3L, 99, 3L, 100, 3L)
        );
        testComputeLong(
                LEVELS,
                List.of(1L, 2L, 3L, 4L, 5L),
                Map.of(1, 1L, 42, 3L, 50, 3L, 80, 5L, 90, 5L, 95, 5L, 99, 5L, 100, 5L)
        );
        testComputeLong(
                LEVELS,
                List.of(1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L, 9L, 10L),
                Map.of(1, 1L, 42, 5L, 50, 6L, 80, 9L, 90, 10L, 95, 10L, 99, 10L, 100, 10L)
        );
    }

    private void testComputeLong(Set<Integer> levels, Collection<Long> values, Map<Integer, Long> expectedPercentiles) {
        var percentiles = new TimingPercentiles(levels);

        var actual = percentiles.compute(values);

        Assertions.assertEquals(expectedPercentiles, actual);
    }
}
