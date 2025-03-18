package ru.yandex.ci.ydb.service.metric;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import io.micrometer.core.instrument.Measurement;
import io.micrometer.core.instrument.Meter;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.simple.SimpleMeterRegistry;
import one.util.streamex.StreamEx;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import ru.yandex.ci.CommonYdbTestBase;

@SuppressWarnings("DoNotMock")
        //Тут осознано мокается только получение времени
class YdbSolomonMetricServiceTest extends CommonYdbTestBase {

    private static final Instant NOW = Instant.now();

    @Test
    void flow() {

        MetricId id1 = MetricId.ofString("id1");
        MetricId id2 = MetricId.ofString("id2");

        MeterRegistry registry = new SimpleMeterRegistry();

        db.currentOrTx(
                () -> {
                    db.metrics().save(id1, 21, NOW.minusSeconds(10));
                    db.metrics().save(id2, 142, NOW.minusSeconds(100500));
                }
        );

        YdbSolomonMetricService metricService = new YdbSolomonMetricService(
                registry,
                db,
                Duration.ofDays(42), //Never in test
                Duration.ofSeconds(100),
                List.of(id1, id2)
        );

        metricService = Mockito.spy(metricService);
        Mockito.when(metricService.now()).thenReturn(NOW);

        metricService.runOneIteration();
        assertMetricValue(registry, id1, 21);
        assertMetricValue(registry, id2, Double.NaN);

        db.currentOrTx(() -> db.metrics().save(id1, 42, NOW.minusSeconds(5)));
        metricService.runOneIteration();
        assertMetricValue(registry, id1, 42);
        assertMetricValue(registry, id2, Double.NaN);

    }

    private void assertMetricValue(MeterRegistry registry, MetricId id, double value) {
        Optional<Meter> meter = StreamEx.of(registry.getMeters())
                .findFirst(m -> m.getId().getName().equals(id.getName()) && m.getId().getTags().equals(id.getTags()));

        Assertions.assertThat(meter).isPresent();
        Assertions.assertThat(
                StreamEx.of(meter.get().measure().iterator()).map(Measurement::getValue).collect(Collectors.toList())
        ).containsExactly(value);
    }
}
