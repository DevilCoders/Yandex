package ru.yandex.ci.storage.reader.check;

import java.time.Instant;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.reader.check.suspicious.RightTimeoutsRule;

import static org.assertj.core.api.Assertions.assertThat;

class CheckAnalysisServiceTest extends StorageYdbTestBase {
    private static final Instant TIMESTAMP = Instant.ofEpochSecond(1602601048L);

    private CheckIterationEntity generateIteration(CheckEntity.Id id, CheckIteration.IterationType iterationType,
                                                   int number, IterationStatistics statistics) {
        return CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(id, iterationType, number))
                .statistics(statistics)
                .created(TIMESTAMP)
                .status(Common.CheckStatus.COMPLETED_SUCCESS)
                .info(IterationInfo.EMPTY)
                .build();
    }

    private MainStatistics.Builder mainStatsBuilder() {
        return MainStatistics.builder()
                .total(StageStatistics.EMPTY)
                .configure(StageStatistics.EMPTY)
                .build(StageStatistics.EMPTY)
                .style(StageStatistics.EMPTY)
                .smallTests(StageStatistics.EMPTY)
                .mediumTests(StageStatistics.EMPTY)
                .largeTests(StageStatistics.EMPTY)
                .teTests(StageStatistics.EMPTY);
    }

    private ExtendedStatistics.Builder extendedStatsBuilder() {
        return ExtendedStatistics.builder()
                .added(ExtendedStageStatistics.EMPTY)
                .deleted(ExtendedStageStatistics.EMPTY)
                .flaky(ExtendedStageStatistics.EMPTY)
                .muted(ExtendedStageStatistics.EMPTY)
                .timeout(ExtendedStageStatistics.EMPTY)
                .external(ExtendedStageStatistics.EMPTY)
                .internal(ExtendedStageStatistics.EMPTY);
    }

    @Test
    void analyzeTimedOutTests() {
        var checkId = CheckEntity.Id.of(1L);
        var iteration1 = generateIteration(checkId, CheckIteration.IterationType.FAST, 1,
                new IterationStatistics(
                        TechnicalStatistics.EMPTY,
                        Metrics.EMPTY,
                        Map.of(TestEntity.ALL_TOOLCHAINS, new IterationStatistics.ToolchainStatistics(
                                mainStatsBuilder()
                                        .build(),
                                extendedStatsBuilder()
                                        .timeout(new ExtendedStageStatistics(182, 12, 56))
                                        .build())
                        )
                )
        );

        var service = new CheckAnalysisService(List.of(
                new RightTimeoutsRule(25, 300)
        ));

        var updated = service.analyzeIterationFinish(iteration1);

        assertThat(updated.getSuspiciousAlerts().size()).isEqualTo(1);
    }
}
