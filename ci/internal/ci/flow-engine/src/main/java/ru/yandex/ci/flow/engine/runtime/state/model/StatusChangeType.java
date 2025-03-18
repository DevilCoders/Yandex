package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.EnumSet;
import java.util.Set;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum StatusChangeType {
    WAITING_FOR_STAGE,
    WAITING_FOR_SCHEDULE,
    QUEUED,
    RUNNING,
    INTERRUPTING,
    KILLED,

    EXECUTOR_SUCCEEDED,
    EXECUTOR_INTERRUPTED,
    EXECUTOR_KILLED,
    FORCED_EXECUTOR_SUCCEEDED,
    EXECUTOR_FAILED,

    EXECUTOR_EXPECTED_FAILED,

    SUBSCRIBERS_SUCCEEDED,
    SUBSCRIBERS_FAILED,

    SUCCESSFUL,
    EXPECTED_FAILED,
    FAILED,
    INTERRUPTED;

    private static final Set<StatusChangeType> TERMINAL_STATUSES = EnumSet.of(
            SUBSCRIBERS_FAILED, SUCCESSFUL, EXPECTED_FAILED, FAILED, INTERRUPTED, KILLED
    );

    private static final Set<StatusChangeType> LIKELY_TO_FINISH_SOON_STATUSES = EnumSet.of(
            EXECUTOR_SUCCEEDED, EXECUTOR_INTERRUPTED, EXECUTOR_KILLED, FORCED_EXECUTOR_SUCCEEDED,
            EXECUTOR_EXPECTED_FAILED,
            EXECUTOR_FAILED, SUBSCRIBERS_SUCCEEDED
    );

    public boolean isFinished() {
        return TERMINAL_STATUSES.contains(this);
    }

    public boolean isSucceeded() {
        return this == SUCCESSFUL;
    }

    public boolean isLikelyToFinishSoon() {
        return LIKELY_TO_FINISH_SOON_STATUSES.contains(this);
    }

    public boolean isFailed() {
        return this == StatusChangeType.FAILED ||
                this == StatusChangeType.EXPECTED_FAILED ||
                this == StatusChangeType.SUBSCRIBERS_FAILED;
    }
}
