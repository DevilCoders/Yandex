package ru.yandex.ci.storage.core.db.model.test_launch;

import java.time.Instant;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.ydb.Persisted;

@Value
@Table(name = "TestLaunches")
@AllArgsConstructor
@Builder(toBuilder = true)
public class TestLaunchEntity implements Entity<TestLaunchEntity> {
    /*
    Additional sql:
    ALTER TABLE `TestLaunches` SET (AUTO_PARTITIONING_MAX_PARTITIONS_COUNT=8192);
    ALTER TABLE `TestLaunches` SET (AUTO_PARTITIONING_PARTITION_SIZE_MB = 2000);
     */

    Id id;

    Common.TestStatus status;
    String uid;
    String processedBy;

    @Column(dbType = DbType.TIMESTAMP)
    Instant processedAt;

    @Column(dbType = DbType.TIMESTAMP)
    Instant revisionCreated;

    @Override
    public TestLaunchEntity.Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<TestLaunchEntity> {
        TestStatusEntity.Id statusId;

        long revisionNumber;

        LaunchRef launch;

        @Override
        public String toString() {
            return "[" + statusId + "/" + revisionNumber + "/" + launch + ']';
        }

        public TestResultEntity.Id toRunId() {
            return new TestResultEntity.Id(
                    launch.getIterationId(), statusId.getFullTestId(), launch.taskId, launch.partition,
                    launch.retryNumber
            );
        }
    }

    @Value
    public static class OldId implements Entity.Id<TestLaunchEntity> {
        CheckIterationEntity.Id iterationId;

        String taskId;
        int partition;
        int retryNumber;
    }

    @Value
    @Persisted
    public static class LaunchRef {
        CheckIterationEntity.Id iterationId;

        String taskId;
        int partition;
        int retryNumber;

        @Override
        public String toString() {
            return "[" + iterationId + '/' + taskId + '/' + partition + '/' + retryNumber + ']';
        }
    }
}
