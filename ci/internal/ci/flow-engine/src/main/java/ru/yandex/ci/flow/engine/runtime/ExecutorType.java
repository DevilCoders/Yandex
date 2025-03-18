package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;

public enum ExecutorType {
    SANDBOX_TASK,
    TASKLET,
    TASKLET_V2,
    // До появления тасклетов на джава, после этого @Deprecated
    CLASS;

    public static ExecutorType selectFor(ExecutorContext executorContext) {
        if (executorContext.getTasklet() != null) {
            return TASKLET;
        }

        if (executorContext.getTaskletV2() != null) {
            return TASKLET_V2;
        }

        if (executorContext.getSandboxTask() != null) {
            return SANDBOX_TASK;
        }

        if (executorContext.getInternal() != null || executorContext.isLegacyInternal()) {
            return CLASS;
        }

        throw new IllegalArgumentException("cannot select executor for context " + executorContext);
    }
}
