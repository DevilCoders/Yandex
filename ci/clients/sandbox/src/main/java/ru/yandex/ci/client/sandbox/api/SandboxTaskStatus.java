package ru.yandex.ci.client.sandbox.api;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;

import ru.yandex.ci.ydb.Persisted;

/**
 * See: https://docs.yandex-team.ru/sandbox/tasks#status
 */
@Persisted
public enum SandboxTaskStatus {
    // end state
    @JsonEnumDefaultValue
    UNKNOWN(SandboxTaskGroupStatus.UNKNOWN),

    NOT_RELEASED(SandboxTaskGroupStatus.FINISH),
    SUCCESS(SandboxTaskGroupStatus.FINISH),
    RELEASED(SandboxTaskGroupStatus.FINISH),
    DELETED(SandboxTaskGroupStatus.FINISH),
    RELEASING(SandboxTaskGroupStatus.FINISH),
    FAILURE(SandboxTaskGroupStatus.FINISH),

    TEMPORARY(SandboxTaskGroupStatus.EXECUTE),
    SUSPENDING(SandboxTaskGroupStatus.EXECUTE),
    ASSIGNED(SandboxTaskGroupStatus.EXECUTE),
    EXECUTING(SandboxTaskGroupStatus.EXECUTE),
    PREPARING(SandboxTaskGroupStatus.EXECUTE),
    SUSPENDED(SandboxTaskGroupStatus.EXECUTE),
    RESUMING(SandboxTaskGroupStatus.EXECUTE),
    STOPPING(SandboxTaskGroupStatus.EXECUTE),
    FINISHING(SandboxTaskGroupStatus.EXECUTE),

    ENQUEUING(SandboxTaskGroupStatus.QUEUE),
    ENQUEUED(SandboxTaskGroupStatus.QUEUE),

    DRAFT(SandboxTaskGroupStatus.DRAFT),

    WAIT_TIME(SandboxTaskGroupStatus.WAIT),
    WAIT_MUTEX(SandboxTaskGroupStatus.WAIT),
    WAIT_OUT(SandboxTaskGroupStatus.WAIT),
    WAIT_RES(SandboxTaskGroupStatus.WAIT),
    WAIT_TASK(SandboxTaskGroupStatus.WAIT),

    NO_RES(SandboxTaskGroupStatus.BREAK),
    EXCEPTION(SandboxTaskGroupStatus.BREAK),
    STOPPED(SandboxTaskGroupStatus.BREAK),
    TIMEOUT(SandboxTaskGroupStatus.BREAK),
    EXPIRED(SandboxTaskGroupStatus.BREAK);

    private final SandboxTaskGroupStatus taskGroupStatus;

    SandboxTaskStatus(SandboxTaskGroupStatus taskGroupStatus) {
        this.taskGroupStatus = taskGroupStatus;
    }

    public SandboxTaskGroupStatus getTaskGroupStatus() {
        return taskGroupStatus;
    }

    public boolean isRunning() {
        return taskGroupStatus == SandboxTaskGroupStatus.EXECUTE ||
                taskGroupStatus == SandboxTaskGroupStatus.QUEUE ||
                taskGroupStatus == SandboxTaskGroupStatus.DRAFT ||
                taskGroupStatus == SandboxTaskGroupStatus.WAIT;
    }

}
