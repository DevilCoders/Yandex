package ru.yandex.ci.engine.registry;

import java.util.Map;

import com.fasterxml.jackson.core.JsonProcessingException;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;

public interface TaskRegistry {
    TaskConfig lookup(ArcRevision revision, TaskId taskId) throws JsonProcessingException;
    Map<TaskId, TaskConfig> loadRegistry(CommitId commitId);
    Map<TaskId, TaskConfig> loadRegistry();
}
