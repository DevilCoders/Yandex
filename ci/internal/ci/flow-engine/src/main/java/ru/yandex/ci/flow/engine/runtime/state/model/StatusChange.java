package ru.yandex.ci.flow.engine.runtime.state.model;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class StatusChange {

    @Nonnull
    StatusChangeType type;

    @Nonnull
    Instant date;

    public static StatusChange successful() {
        return of(StatusChangeType.SUCCESSFUL);
    }

    public static StatusChange failed() {
        return of(StatusChangeType.FAILED);
    }

    public static StatusChange expectedFailed() {
        return of(StatusChangeType.EXPECTED_FAILED);
    }

    public static StatusChange interrupting() {
        return of(StatusChangeType.INTERRUPTING);
    }

    public static StatusChange executorInterrupted() {
        return of(StatusChangeType.EXECUTOR_INTERRUPTED);
    }

    public static StatusChange executorKilled() {
        return of(StatusChangeType.EXECUTOR_KILLED);
    }

    public static StatusChange interrupted() {
        return of(StatusChangeType.INTERRUPTED);
    }

    public static StatusChange killed() {
        return of(StatusChangeType.KILLED);
    }

    public static StatusChange executorSucceeded() {
        return of(StatusChangeType.EXECUTOR_SUCCEEDED);
    }

    public static StatusChange forcedExecutorSucceeded() {
        return of(StatusChangeType.FORCED_EXECUTOR_SUCCEEDED);
    }

    public static StatusChange executorFailed() {
        return of(StatusChangeType.EXECUTOR_FAILED);
    }

    public static StatusChange executorExpectedFailed() {
        return of(StatusChangeType.EXECUTOR_EXPECTED_FAILED);
    }

    public static StatusChange subscribersSucceeded() {
        return of(StatusChangeType.SUBSCRIBERS_SUCCEEDED);
    }

    public static StatusChange subscribersFailed() {
        return of(StatusChangeType.SUBSCRIBERS_FAILED);
    }

    public static StatusChange running() {
        return of(StatusChangeType.RUNNING);
    }

    public static StatusChange queued() {
        return of(StatusChangeType.QUEUED);
    }

    public static StatusChange waitingForStage() {
        return of(StatusChangeType.WAITING_FOR_STAGE);
    }

    public static StatusChange waitngForSchedule() {
        return of(StatusChangeType.WAITING_FOR_SCHEDULE);
    }

    private static StatusChange of(StatusChangeType type) {
        return new StatusChange(type, Instant.now());
    }
}
