package ru.yandex.ci.observer.core.db.model.check_iterations;

import java.time.Instant;

import lombok.Value;

import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class CheckIterationNumberStagesAggregation {
    int number;
    DurationStages stagesAggregation;
    Instant created;
    Instant finish;
    Common.CheckStatus status;

    public static CheckIterationNumberStagesAggregation of(CheckIterationEntity iteration) {
        return new CheckIterationNumberStagesAggregation(
                iteration.getId().getNumber(),
                iteration.getStagesAggregation(),
                iteration.getCreated(),
                iteration.getFinish(),
                iteration.getStatus()
        );
    }
}
