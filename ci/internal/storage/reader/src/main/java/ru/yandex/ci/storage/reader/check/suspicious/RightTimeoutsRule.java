package ru.yandex.ci.storage.reader.check.suspicious;

import java.util.function.Consumer;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@Slf4j
public class RightTimeoutsRule implements CheckAnalysisRule {
    private final int totalPercent;
    private final int rightPercent;

    public RightTimeoutsRule(int totalPercent, int rightPercent) {
        this.totalPercent = totalPercent;
        this.rightPercent = rightPercent;
    }

    @Override
    public void analyzeIteration(CheckIterationEntity iteration, Consumer<String> addAlert) {
        var extended = iteration.getStatistics().getAllToolchain().getExtended();
        if (extended == null || extended.getTimeout() == null) {
            return;
        }

        var totalTimeouts = extended.getTimeout().getTotal();
        var rightTimeouts = extended.getTimeout().getFailedAdded();
        var leftTimeouts = extended.getTimeout().getPassedAdded();

        var suspicious = rightTimeouts * 100 > leftTimeouts * rightPercent &&
                (rightTimeouts + leftTimeouts) * 100 > totalTimeouts * totalPercent;

        if (suspicious) {
            log.info(
                    "Suspicious {}: {} right timeouts, {} left, {} total",
                    iteration.getId(), rightTimeouts, leftTimeouts, totalTimeouts
            );

            addAlert.accept("%d new timeouts".formatted(rightTimeouts));
        }
    }
}
