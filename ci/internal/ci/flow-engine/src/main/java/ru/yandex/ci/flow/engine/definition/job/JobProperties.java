package ru.yandex.ci.flow.engine.definition.job;

import lombok.Value;

import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.launch.TaskVersion;

/**
 * Контейнер с информацией о джобе, которая не используется в движке графа,
 * но полезна другим потребителям флоу.
 */
@Value(staticConstructor = "of")
public class JobProperties {
    TaskId taskId;
    TaskVersion taskVersion;
}
