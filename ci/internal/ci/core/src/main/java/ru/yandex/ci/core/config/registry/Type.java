package ru.yandex.ci.core.config.registry;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@Persisted
public enum Type {
    TASKLET,
    TASKLET_V2,
    SANDBOX_TASK,
    INTERNAL_TASK,
    DUMMY;

    public static Type of(@Nullable TaskConfig config, TaskId id) {
        if (id.isDummy()) {
            return DUMMY;
        }

        Preconditions.checkNotNull(config);
        if (config.getTasklet() != null) {
            return TASKLET;
        }
        if (config.getTaskletV2() != null) {
            return TASKLET_V2;
        }
        if (config.getSandboxTask() != null) {
            return SANDBOX_TASK;
        }
        if (config.getInternalTask() != null) {
            return INTERNAL_TASK;
        }

        throw new IllegalArgumentException("cannot get type of task " + config);
    }
}
