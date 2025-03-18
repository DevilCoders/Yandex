package ru.yandex.ci.storage.reader.other;

import java.util.Map;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metric;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;

import static org.assertj.core.api.AssertionsForClassTypes.assertThat;
import static org.mockito.Mockito.mock;

public class MetricAggregateServiceTest {
    private final MetricAggregationService service = new MetricAggregationService(
            new ReaderStatistics(
                    mock(MainStreamStatistics.class),
                    mock(ShardOutStreamStatistics.class),
                    mock(EventsStreamStatistics.class),
                    null
            )
    );

    @Test
    void update() {
        var original = new Metrics(Map.of(
                "time", time(19),
                "memory_usage", mem(9)
        ));

        assertThat(service.merge(original, Metrics.EMPTY)).isEqualTo(original);
        assertThat(service.merge(original,
                new Metrics(Map.of(
                        "slept", time(9),
                        "disk", mem(91)
                ))))
                .isEqualTo(new Metrics(Map.of(
                        "time", time(19),
                        "slept", time(9),
                        "memory_usage", mem(9),
                        "disk", mem(91)
                )));

        assertThat(service.merge(original,
                new Metrics(Map.of(
                        "time", time(2),
                        "memory_usage", mem(5)
                ))))
                .isEqualTo(new Metrics(Map.of(
                        "time", time(19),
                        "memory_usage", mem(9)
                )));
    }


    @Test
    void aggregate() {
        var original = new Metrics(Map.of(
                "time", time(19),
                "memory_usage", mem(9)
        ));

        assertThat(service.aggregate(original, Metrics.EMPTY, Metrics.EMPTY)).isEqualTo(original);
        assertThat(service.aggregate(original, original, original)).isEqualTo(original);

        assertThat(service.aggregate(
                original,
                new Metrics(Map.of()),
                new Metrics(Map.of(
                        "slept", time(9),
                        "disk", mem(91)
                ))))
                .isEqualTo(new Metrics(Map.of(
                        "time", time(19),
                        "slept", time(9),
                        "memory_usage", mem(9),
                        "disk", mem(91)
                )));

        assertThat(service.aggregate(
                original,
                new Metrics(Map.of(
                        "time", time(9),
                        "memory_usage", mem(5)
                )),
                new Metrics(Map.of(
                        "time", time(10),
                        "memory_usage", mem(6)
                ))))
                .isEqualTo(new Metrics(Map.of(
                        "time", time(20),
                        "memory_usage", mem(9)
                )));

        assertThat(service.aggregate(
                original,
                new Metrics(Map.of()),
                new Metrics(Map.of(
                        "time", time(2)
                ))))
                .isEqualTo(new Metrics(Map.of(
                        "time", time(21),
                        "memory_usage", mem(9)
                )));
    }

    private static Metric time(int value) {
        return new Metric(value, Common.MetricAggregateFunction.SUM, Common.MetricSize.SECONDS);
    }

    private static Metric mem(int value) {
        return new Metric(value, Common.MetricAggregateFunction.MAX, Common.MetricSize.BYTES);
    }

    @ParameterizedTest(name = "aggregate {0} a = {1}, b = {2}, expected = {3}")
    @CsvSource(
            textBlock = """
                    MAX, 17, 29, 29
                    MAX, 17, 8,  17
                    MAX, 17, 23, 23
                    SUM, 17, 29, 46
                    SUM, 17, 13, 30
                    """
    )
    void aggregate(Common.MetricAggregateFunction function, int a, int b, int expected) {
        assertThat(service.aggregate(bytesMetric(a, function), null, bytesMetric(b, function)))
                .isEqualTo(bytesMetric(expected, function));
    }

    @ParameterizedTest(name = "update {0} agg = {1}, old = {2}, updated = {3}, expected = {4}")
    @CsvSource(
            textBlock = """
                    MAX, 17,  , 29, 29
                    MAX, 17,  , 8,  17
                    MAX, 17, 9, 13, 17
                    MAX, 17, 9, 23, 23
                    SUM, 17,  , 29, 46
                    SUM, 17, 9, 13, 21
                    SUM, 17, 9, 32, 40
                    """
    )
    void update(Common.MetricAggregateFunction function, int aggregated, Integer old, int updated, int expected) {
        assertThat(service.aggregate(
                bytesMetric(aggregated, function), bytesMetric(old, function), bytesMetric(updated, function))
        ).isEqualTo(bytesMetric(expected, function));
    }

    @Nullable
    private static Metric bytesMetric(@Nullable Integer value, Common.MetricAggregateFunction function) {
        if (value == null) {
            return null;
        }
        return new Metric(value, function, Common.MetricSize.BYTES);
    }
}
