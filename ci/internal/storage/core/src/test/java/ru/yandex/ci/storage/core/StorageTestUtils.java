package ru.yandex.ci.storage.core;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;

public class StorageTestUtils {
    private StorageTestUtils() {

    }

    public static CheckEntity.Id createCheckId() {
        return CheckEntity.Id.of(1L);
    }

    public static CheckIterationEntity.Id createIterationId() {
        return createIterationId(createCheckId());
    }

    public static CheckIterationEntity.Id createIterationId(CheckEntity.Id checkId) {
        return CheckIterationEntity.Id.of(checkId, CheckIteration.IterationType.FULL, 1);
    }

    public static CheckTaskEntity.Id createTaskId(CheckIterationEntity.Id iterationId) {
        return createTaskId(iterationId, "task_id");
    }

    public static CheckTaskEntity.Id createTaskId(CheckIterationEntity.Id iterationId, String taskId) {
        return new CheckTaskEntity.Id(iterationId, taskId);
    }

    public static ChunkAggregateEntity createAggregate() {
        return ChunkAggregateEntity.create(
                new ChunkAggregateEntity.Id(createIterationId(), ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1))
        );
    }
}
