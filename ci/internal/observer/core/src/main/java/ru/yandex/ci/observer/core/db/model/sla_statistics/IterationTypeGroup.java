package ru.yandex.ci.observer.core.db.model.sla_statistics;

import com.google.common.collect.ImmutableList;
import lombok.Getter;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.ydb.Persisted;

@Getter
@Persisted
public enum IterationTypeGroup {
    FAST(ImmutableList.of(CheckIteration.IterationType.FAST)),
    FULL(ImmutableList.of(CheckIteration.IterationType.FULL)),
    ANY(ImmutableList.of(CheckIteration.IterationType.FAST, CheckIteration.IterationType.FULL));

    private final ImmutableList<CheckIteration.IterationType> iterationTypes;

    IterationTypeGroup(ImmutableList<CheckIteration.IterationType> iterationTypes) {
        this.iterationTypes = iterationTypes;
    }
}
