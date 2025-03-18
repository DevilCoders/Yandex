package ru.yandex.ci.storage.core.db.model.test_diff;

import java.time.Instant;
import java.util.Objects;

import javax.annotation.Nullable;

import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Table(name = "TestImportantDiffs")
public class TestDiffImportantEntity implements Entity<TestDiffImportantEntity> {
    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestDiffImportantEntity.class, 1024,
                partitions -> ydbTableHint = HintRegistry.byCheckEntityPoints(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMaxPartitionsCount(4096)
                )
        );
    }

    Id id;

    @Nullable
    @Column(dbType = DbType.UINT64)
    Long autocheckChunkId;

    String name;
    String subtestName;
    String processedBy;
    String strongModeAYaml;
    String oldTestId;
    String oldSuiteId;

    boolean uidChanged;

    String tags;
    String requirements;

    TestDiffType diffType;

    TestStatus left;
    TestStatus right;

    Integer leftIterationNumber;
    Integer rightIterationNumber;

    @Column(dbType = DbType.UINT8)
    Integer clonedFromIteration;

    boolean isMuted;
    Boolean isStrongMode;
    boolean isLast; // last diff over all iterations of this type
    @Nullable
    Boolean isLaunchable;

    @Column(dbType = "UINT32")
    @Nullable
    Integer aggregateIdHash; // for link to TestDiffByHashEntity

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    TestDiffStatistics statistics;

    @Nullable
    ChunkEntity.Id chunkId;

    @Nullable
    Boolean isOwner;

    @Nullable
    @Column(flatten = false)
    ResultOwners owners; // old owners for notifications

    public TestDiffImportantEntity(TestDiffByHashEntity entity) {
        this.id = new Id(
                entity.getId().getAggregateId().getIterationId(),
                entity.getResultType(),
                entity.getPath(),
                entity.getId().getTestId()
        );
        this.autocheckChunkId = entity.getAutocheckChunkId();
        this.name = entity.getName();
        this.subtestName = entity.getSubtestName();
        this.uidChanged = entity.isUidChanged();
        this.tags = TagUtils.toTagsString(entity.getTags());
        this.requirements = entity.getRequirements();
        this.diffType = entity.getDiffType();
        this.left = entity.getLeft();
        this.right = entity.getRight();
        this.leftIterationNumber = entity.getLeftIterationNumber();
        this.rightIterationNumber = entity.getRightIterationNumber();
        this.clonedFromIteration = entity.getClonedFromIteration();
        this.isLast = entity.isLast();
        this.isStrongMode = entity.isStrongMode();
        this.isLaunchable = entity.getIsLaunchable();
        this.oldTestId = entity.getOldTestId();
        this.oldSuiteId = entity.getOldSuiteId();
        this.strongModeAYaml = entity.getStrongModeAYaml();
        this.isMuted = entity.isMuted();
        this.created = entity.getCreated();
        this.statistics = entity.getStatistics();
        this.processedBy = entity.getProcessedBy();
        this.aggregateIdHash = entity.getId().getAggregateIdHash();
        this.chunkId = entity.getId().getAggregateId().getChunkId();
        this.isOwner = entity.getIsOwner();
        this.owners = entity.getOwners();
    }

    @Override
    public Id getId() {
        return id;
    }

    public boolean isStrongMode() {
        return isStrongMode != null && isStrongMode;
    }

    public String getStrongModeAYaml() {
        return Objects.requireNonNullElse(strongModeAYaml, "");
    }

    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestDiffImportantEntity> {
        CheckEntity.Id checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        String toolchain;

        ResultType resultType;

        String path;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        @Column(dbType = DbType.UINT64)
        long testId;

        // split from iteration id to search over all iterations using pk index.
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

        public TestEntity.Id getCombinedTestId() {
            return new TestEntity.Id(suiteId, toolchain, testId);
        }

        @Override
        public String toString() {
            return "[%s]/%s/[%s]".formatted(
                    getIterationId(), resultType, getCombinedTestId()
            );
        }
    }
}

