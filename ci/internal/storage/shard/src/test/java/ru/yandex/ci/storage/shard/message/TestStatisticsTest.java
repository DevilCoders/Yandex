package ru.yandex.ci.storage.shard.message;

import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.util.Map;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatistics;

import static org.assertj.core.api.Assertions.assertThat;

public class TestStatisticsTest extends CommonTestBase {

    @Test
    public void countsNonFlakyDays() {
        var statistics = new TestStatistics(
                0,
                0,
                Map.of(
                        LocalDateTime.of(2000, 1, 1, 1, 1, 1, 1),
                        TestStatistics.RunStatistics.EMPTY,
                        LocalDateTime.of(2000, 1, 2, 1, 1, 1, 1),
                        new TestStatistics.RunStatistics(
                                Map.of(
                                        Common.TestStatus.TS_FLAKY, 1
                                )
                        ),
                        LocalDateTime.of(2000, 1, 3, 1, 1, 1, 1),
                        TestStatistics.RunStatistics.EMPTY,
                        LocalDateTime.of(2000, 1, 4, 1, 1, 1, 1),
                        TestStatistics.RunStatistics.EMPTY
                )
        );

        var updated = statistics.onRun(
                Common.TestStatus.TS_OK, LocalDateTime.of(2000, 1, 5, 1, 1, 1, 1).toInstant(ZoneOffset.UTC)
        );

        assertThat(updated.getNoneFlakyDays()).isEqualTo(3);
    }
}
