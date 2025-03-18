package ru.yandex.ci.core.config.registry;

import com.google.common.base.Preconditions;
import lombok.EqualsAndHashCode;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@EqualsAndHashCode
public class TaskId implements Comparable<TaskId> {
    public static final String DUMMY_TASK = "dummy";

    private final String id;

    private TaskId(String id) {
        Preconditions.checkNotNull(id);
        Preconditions.checkArgument(!id.startsWith("/"), "id should omit a leading slash /");

        this.id = id;
    }

    public static TaskId of(String path) {
        return new TaskId(path);
    }

    public String getId() {
        return id;
    }

    /**
     * Пустой узел без какой-либо работы
     */
    public boolean isDummy() {
        return id.equalsIgnoreCase(DUMMY_TASK);
    }

    /**
     * Требуется наличие конфигурации в реестре. Некоторые задачи не требуют этого, например {@link #isDummy()}
     */
    public boolean requireTaskConfig() {
        return !isDummy();
    }

    @Override
    public int compareTo(TaskId o) {
        return id.compareTo(o.id);
    }


    @Override
    public String toString() {
        return getId();
    }
}
