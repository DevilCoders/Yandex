package ru.yandex.ci.storage.core.db.model.test_status;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class TestStatistics {
    private static final int NUMBER_OF_DAYS = 14;
    private static final List<Common.TestStatus> FAIL_STATUSES = List.of(
            Common.TestStatus.TS_FAILED,
            Common.TestStatus.TS_FLAKY,
            Common.TestStatus.TS_INTERNAL,
            Common.TestStatus.TS_TIMEOUT,
            Common.TestStatus.TS_BROKEN_DEPS,
            Common.TestStatus.TS_XPASSED,
            Common.TestStatus.TS_SUITE_PROBLEMS
    );

    public static final TestStatistics EMPTY = new TestStatistics(0, 0, Map.of());

    float failureScore;

    int noneFlakyDays;

    Map<LocalDateTime, RunStatistics> days;

    @Persisted
    @Value
    public static class RunStatistics {
        public static final RunStatistics EMPTY = new RunStatistics(Map.of());

        Map<Common.TestStatus, Integer> runs;

        public RunStatistics onRun(Common.TestStatus status) {
            var newRuns = new HashMap<>(runs);

            newRuns.compute(status, (k, v) -> (v == null) ? 1 : v + 1);

            return new RunStatistics(Collections.unmodifiableMap(newRuns));
        }
    }

    public TestStatistics onRun(Common.TestStatus status, Instant timestamp) {
        var instant = timestamp.atZone(ZoneOffset.UTC).toLocalDate().atStartOfDay();
        LocalDateTime min = null;
        if (days.size() == NUMBER_OF_DAYS) {
            min = days.keySet().stream().min(Comparator.comparing(Function.identity())).get();
            if (instant.isBefore(min)) {
                return this;
            }
        }

        var newDays = new HashMap<>(days);
        if (min != null) {
            newDays.remove(min);
        }

        var statistics = newDays.getOrDefault(instant, RunStatistics.EMPTY);
        newDays.put(instant, statistics.onRun(status));

        var totalRuns = (float) newDays.values().stream()
                .flatMap(x -> x.getRuns().values().stream())
                .mapToInt(x -> x)
                .sum();

        var failRuns = (float) newDays.values().stream()
                .flatMap(x -> x.getRuns().entrySet().stream())
                .filter(e -> FAIL_STATUSES.contains(e.getKey()))
                .mapToInt(Map.Entry::getValue)
                .sum();

        var lastFlakyDay = newDays.entrySet().stream()
                .filter(x -> x.getValue().getRuns().getOrDefault(Common.TestStatus.TS_FLAKY, 0) > 0)
                .map(Map.Entry::getKey)
                .max(Comparator.comparing(Function.identity())).orElse(null);

        var newNoneFlakyDays = lastFlakyDay == null ?
                newDays.size() : (int) newDays.keySet().stream().filter(x -> x.isAfter(lastFlakyDay)).count();

        return new TestStatistics(
                failRuns / totalRuns,
                newNoneFlakyDays,
                Collections.unmodifiableMap(newDays)
        );
    }
}
