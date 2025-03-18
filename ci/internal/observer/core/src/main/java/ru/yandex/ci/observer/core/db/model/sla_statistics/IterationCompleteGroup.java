package ru.yandex.ci.observer.core.db.model.sla_statistics;

import com.google.common.collect.ImmutableList;
import lombok.Getter;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Getter
@Persisted
public enum IterationCompleteGroup {
    PASSED(ImmutableList.of(Common.CheckStatus.COMPLETED_SUCCESS)),
    FAILED(ImmutableList.of(
            Common.CheckStatus.COMPLETED_FAILED,
            Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR
    )),
    ANY(ImmutableList.of(
            Common.CheckStatus.COMPLETED_SUCCESS,
            Common.CheckStatus.COMPLETED_FAILED,
            Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR
    ));

    private final ImmutableList<Common.CheckStatus> statuses;

    IterationCompleteGroup(ImmutableList<Common.CheckStatus> statuses) {
        this.statuses = statuses;
    }
}
