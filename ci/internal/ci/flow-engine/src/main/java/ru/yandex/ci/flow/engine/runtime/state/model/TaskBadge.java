package ru.yandex.ci.flow.engine.runtime.state.model;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class TaskBadge {

    private static final String RESERVED_TASK_PREFIX = "ci:";

    public static final String DEFAULT_ID = "default";
    public static final float ZERO_PROGRESS = 0.0f;
    public static final float HALF_PROGRESS = 0.5f;
    public static final float FULL_PROGRESS = 1.0f;

    @Nonnull
    String id;

    @Nonnull
    String module;

    @Nullable
    String url;

    @Nonnull
    @With
    TaskStatus status;

    @Nullable
    Float progress;

    @Nullable
    String text;

    boolean primary;

    public static void checkNotReservedId(String id) {
        Preconditions.checkArgument(
                !id.startsWith(RESERVED_TASK_PREFIX),
                "task ids fix prefix \"%s\" are reserved",
                RESERVED_TASK_PREFIX
        );
    }

    public static String reservedTaskId(String id) {
        return RESERVED_TASK_PREFIX + id;
    }

    public static TaskBadge of(String id, String module, String url, TaskStatus status) {
        return of(id, module, url, status, null, null, false);
    }

    @Persisted
    public enum TaskStatus {
        RUNNING,
        FAILED,
        SUCCESSFUL
    }
}
