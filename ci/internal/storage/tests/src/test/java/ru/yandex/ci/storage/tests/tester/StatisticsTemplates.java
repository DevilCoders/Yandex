package ru.yandex.ci.storage.tests.tester;

import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;

public class StatisticsTemplates {

    private StatisticsTemplates() {

    }

    public static ExtendedStageStatistics extendedFailedAdded(int number) {
        return ExtendedStageStatistics.builder()
                .total(number)
                .failedAdded(number)
                .build();
    }

    public static ExtendedStageStatistics extendedPassedAdded(int number) {
        return ExtendedStageStatistics.builder()
                .total(number)
                .passedAdded(number)
                .build();
    }

    public static StageStatistics stageFailedAdded(int number) {
        return StageStatistics.builder()
                .total(number)
                .failed(number)
                .failedAdded(number)
                .build();
    }

    public static StageStatistics stageTotal(int number) {
        return StageStatistics.builder()
                .total(number)
                .build();
    }

    public static StageStatistics stagePassed(int number) {
        return StageStatistics.builder()
                .total(number)
                .passed(number)
                .build();
    }

    public static MainStatistics mainBuild(StageStatistics stage) {
        return MainStatistics.builder()
                .total(stage)
                .build(stage)
                .build();
    }

    public static Object mainMedium(StageStatistics stage) {
        return MainStatistics.builder()
                .total(stage)
                .mediumTests(stage)
                .build();
    }

    public static Object mainLarge(StageStatistics stage) {
        return MainStatistics.builder()
                .total(stage)
                .largeTests(stage)
                .build();
    }

    public static ExtendedStatistics timeoutAdded(int number) {
        return ExtendedStatistics.builder()
                .timeout(StatisticsTemplates.extendedFailedAdded(number))
                .build();
    }


}
