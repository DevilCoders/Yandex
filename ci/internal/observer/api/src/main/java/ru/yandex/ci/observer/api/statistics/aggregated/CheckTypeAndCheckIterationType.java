package ru.yandex.ci.observer.api.statistics.aggregated;

import lombok.Value;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationsTable;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Value(staticConstructor = "of")
public class CheckTypeAndCheckIterationType {
    CheckOuterClass.CheckType checkType;
    CheckIteration.IterationType iterationType;

    public static CheckTypeAndCheckIterationType of(CheckIterationsTable.WindowedStatsByPoolCount stats) {
        return CheckTypeAndCheckIterationType.of(stats.getCheckType(), stats.getIterationType());
    }

    public static CheckTypeAndCheckIterationType of(
            CheckIterationsTable.CountByCheckTypeAdvisedPoolIterationType count) {
        return CheckTypeAndCheckIterationType.of(count.getCheckType(), count.getIterationType());
    }
}
