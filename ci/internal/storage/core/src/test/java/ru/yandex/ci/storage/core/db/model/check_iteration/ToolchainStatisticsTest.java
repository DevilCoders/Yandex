package ru.yandex.ci.storage.core.db.model.check_iteration;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;

import static org.assertj.core.api.Assertions.assertThat;

public class ToolchainStatisticsTest {

    @Test
    void toCompletedStatus_shouldReturnCompletedFailed_whenFailedAdded() {
        assertThat(evaluateCheckStatus(
                StageStatistics.builder().failedAdded(1).build()
        )).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);
    }

    @Test
    void toCompletedStatus_whenFailed() {
        assertThat(evaluateCheckStatus(
                StageStatistics.builder()
                        .failed(1)
                        .failedInStrongMode(1)
                        .build()
        )).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);
        assertThat(evaluateCheckStatus(
                StageStatistics.builder()
                        .failed(1)
                        .build()
        )).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
    }

    private static Common.CheckStatus evaluateCheckStatus(StageStatistics stageStatistics) {
        return new IterationStatistics.ToolchainStatistics(
                MainStatistics.builder()
                        .total(stageStatistics)
                        .build(),
                ExtendedStatistics.EMPTY
        ).toCompletedStatus();
    }

}
