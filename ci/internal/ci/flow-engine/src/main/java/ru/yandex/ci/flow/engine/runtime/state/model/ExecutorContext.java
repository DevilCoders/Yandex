package ru.yandex.ci.flow.engine.runtime.state.model;

import java.time.Duration;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.primitives.Booleans;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.sandbox.SandboxTaskRef;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.ydb.Persisted;

/**
 * Контейнер настроек executor-ов конкретной джобы. Должен содержать только свойства графа, а не запуска.
 * По его содержимому выбирается конкретный executor.
 */
@SuppressWarnings("ReferenceEquality")
@Persisted
@Value
@Getter(AccessLevel.NONE)
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class ExecutorContext {
    @Nullable
    TaskletExecutorContext tasklet;

    @Nullable
    TaskletV2ExecutorContext taskletV2;

    @Nullable
    SandboxExecutorContext sandboxTask;

    @Nullable
    InternalExecutorContext internal;

    @Nullable
    @With
    RequirementsConfig requirements;

    @Nullable
    Duration killTimeout;

    @With
    @Nullable
    RuntimeConfig jobRuntimeConfig;

    @Nullable
    @Deprecated
    SandboxTaskRef sandboxTaskRef;

    @Nullable
    @Deprecated
    String executorClassName;

    public ExecutorContext(
            @Nullable InternalExecutorContext internal,
            @Nullable TaskletExecutorContext tasklet,
            @Nullable TaskletV2ExecutorContext taskletV2,
            @Nullable SandboxExecutorContext sandboxTask,
            @Nullable RequirementsConfig requirements,
            @Nullable RuntimeConfig jobRuntimeConfig
    ) {
        this.internal = internal;
        this.tasklet = tasklet;
        this.taskletV2 = taskletV2;
        this.sandboxTask = sandboxTask;
        this.requirements = requirements;

        this.killTimeout = null;
        this.jobRuntimeConfig = jobRuntimeConfig;
        this.sandboxTaskRef = null;
        this.executorClassName = null;

        Preconditions.checkArgument(
                Booleans.countTrue(
                        tasklet != null,
                        taskletV2 != null,
                        sandboxTask != null,
                        internal != null
                ) == 1,
                "Should by exactly one context provided, but %s",
                this
        );
    }

    @Nullable
    public TaskletExecutorContext getTasklet() {
        return tasklet;
    }

    @Nullable
    public TaskletV2ExecutorContext getTaskletV2() {
        return taskletV2;
    }

    @Nullable
    public SandboxExecutorContext getSandboxTask() {
        // backward compatibility
        if (sandboxTask == null && sandboxTaskRef != null) {
            return SandboxExecutorContext.of(sandboxTaskRef.getType());
        } else {
            return sandboxTask;
        }
    }

    @Nullable
    public InternalExecutorContext getInternal() {
        return internal;
    }

    public boolean isLegacyInternal() {
        return executorClassName != null && internal == null;
    }

    @Nullable
    public RequirementsConfig getRequirements() {
        return requirements;
    }

    @Nullable
    public RuntimeConfig getJobRuntimeConfig() {
        return jobRuntimeConfig != null
                ? jobRuntimeConfig
                :
                (killTimeout != null
                        ? RuntimeConfig.ofKillTimeout(killTimeout)
                        : null);
    }

    public static ExecutorContext internal(InternalExecutorContext internal) {
        return new ExecutorContext(internal, null, null, null, null, null);
    }

}
