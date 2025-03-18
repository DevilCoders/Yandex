package ru.yandex.ci.storage.core.db.model.test_diff.index;

import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value
@Builder(toBuilder = true)
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Table(name = "TestDiffsBySuite")
public class TestDiffBySuiteEntity implements Entity<TestDiffBySuiteEntity> {
    /*
    Additional sql:
    ALTER TABLE `TestDiffsBySuite` SET (AUTO_PARTITIONING_PARTITION_SIZE_MB = 2000);
    ALTER TABLE `TestDiffsBySuite` SET (AUTO_PARTITIONING_BY_LOAD = ENABLED);
    ALTER TABLE `TestDiffsBySuite` SET (AUTO_PARTITIONING_MAX_PARTITIONS_COUNT=8000);
     */
    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestDiffBySuiteEntity.class, 100,
                partitions -> ydbTableHint = HintRegistry.byCheckEntityPoints(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMaxPartitionsCount(8000)
                                .setPartitionSize(2000)
                )
        );
    }

    Id id;

    @Column(dbType = "UINT32")
    Integer aggregateIdHash;

    ChunkEntity.Id chunkId;

    public TestDiffBySuiteEntity(TestDiffByHashEntity entity) {
        this.id = new Id(
                entity.getId().getAggregateId().getIterationId(),
                entity.getResultType(),
                entity.getPath(),
                entity.getId().getTestId()
        );
        this.aggregateIdHash = entity.getId().getAggregateIdHash();
        this.chunkId = entity.getId().getAggregateId().getChunkId();
    }


    @Override
    public Id getId() {
        return id;
    }

    @Value
    @lombok.Builder
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestDiffBySuiteEntity> {
        CheckEntity.Id checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        String toolchain;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        @Column(dbType = DbType.UINT64)
        long testId;

        String path;

        ResultType resultType;

        @Column(dbType = DbType.UINT8)
        int iterationNumber;

        public Id(CheckIterationEntity.Id iterationId, ResultType resultType, String path, TestEntity.Id testId) {
            this.checkId = iterationId.getCheckId();
            this.iterationType = iterationId.getIterationType().getNumber();
            this.iterationNumber = iterationId.getNumber();
            this.resultType = resultType;
            this.path = path;
            this.toolchain = testId.getToolchain();
            this.suiteId = testId.getSuiteId();
            this.testId = testId.getId();
        }

        public CheckIterationEntity.Id getIterationId() {
            return new CheckIterationEntity.Id(checkId, iterationType, iterationNumber);
        }

        public TestEntity.Id getFullTestId() {
            return new TestEntity.Id(suiteId, toolchain, testId);
        }
    }
}
