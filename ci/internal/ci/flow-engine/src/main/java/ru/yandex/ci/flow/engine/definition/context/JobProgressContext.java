package ru.yandex.ci.flow.engine.definition.context;

import java.util.Collection;
import java.util.function.Consumer;

import ru.yandex.ci.flow.engine.definition.context.impl.JobProgressContextImpl;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

public interface JobProgressContext {
    void update(Consumer<JobProgressContextImpl.ProgressBuilder> callback);

    default void updateText(String text) {
        update(progress -> progress.setText(text));
    }

    TaskBadge getTaskState(String module);

    TaskBadge getTaskState(long index, String module);

    Collection<TaskBadge> getTaskStates();

    void updateTaskState(TaskBadge taskBadge);
}
