package ru.yandex.ci.storage.core.db.model.test_launch;

import lombok.AllArgsConstructor;
import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

// Imaginary entity, not exists in db.
@Value
@Table(name = "Test short launch [imaginary]")
public class TestShortLaunchEntity implements Entity<TestShortLaunchEntity> {
    Id id;

    @Override
    public Entity.Id<TestShortLaunchEntity> getId() {
        return id;
    }

    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestShortLaunchEntity> {
        long checkId;
        int taskIdHash;

        public Id(TaskPartitionRetryId id) {
            this.checkId = id.taskId.getIterationId().getCheckId().getId();
            this.taskIdHash = id.hashCode();
        }
    }

    @Value
    public static class TaskPartitionRetryId implements Entity.Id<TestShortLaunchEntity> {
        CheckTaskEntity.Id taskId;
        int partition;
        int retryNumber;
    }
}
