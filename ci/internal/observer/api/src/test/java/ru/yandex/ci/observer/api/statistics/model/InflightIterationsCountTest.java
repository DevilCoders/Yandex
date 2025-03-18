package ru.yandex.ci.observer.api.statistics.model;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.observer.api.ObserverApiYdbTestBase;
import ru.yandex.ci.storage.core.CheckIteration;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.observer.api.statistics.model.SensorLabels.ANY_CHECK_ITERATION_TYPE;
import static ru.yandex.ci.observer.api.statistics.model.SensorLabels.ANY_POOL;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.TRUNK_PRE_COMMIT;

class InflightIterationsCountTest extends ObserverApiYdbTestBase {

    private static final Instant FROM = TIME;
    private static final Instant TO = FROM.plus(200, ChronoUnit.MINUTES);

    @Test
    void compute_whenNoInflights() {
        assertThat(InflightIterationsCount.compute(FROM, TO, List.of(TRUNK_PRE_COMMIT), db))
                .containsExactlyInAnyOrder(
                        new InflightIterationsCount(TRUNK_PRE_COMMIT, ANY_POOL, CheckIteration.IterationType.FAST, 0),
                        new InflightIterationsCount(TRUNK_PRE_COMMIT, ANY_POOL, CheckIteration.IterationType.FULL, 0),
                        new InflightIterationsCount(TRUNK_PRE_COMMIT, ANY_POOL, ANY_CHECK_ITERATION_TYPE, 0)
                );
    }
}
