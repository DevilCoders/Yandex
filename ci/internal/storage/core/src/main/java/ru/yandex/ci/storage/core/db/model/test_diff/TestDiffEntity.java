package ru.yandex.ci.storage.core.db.model.test_diff;

import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

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
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value
@Builder(toBuilder = true)
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Table(name = "TestDiffs")
public class TestDiffEntity implements Entity<TestDiffEntity> {
    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestDiffEntity.class, 1024,
                partitions -> ydbTableHint = HintRegistry.byCheckEntityPoints(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMinPartitionsCount(Math.min(partitions, 6000))
                                .setMaxPartitionsCount(18000)
                                .setPartitionSize(2000)
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

    @Nullable
    String oldTestId;

    @Nullable
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

    @Nullable // old values
    Boolean important; // will be copied to TestDiffImportantEntity

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

    public TestDiffEntity(TestDiffByHashEntity entity) {
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
        this.important = entity.getImportant();
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

    public TestDiffEntity(TestDiffImportantEntity entity) {
        this.id = new Id(entity.getId());
        this.autocheckChunkId = entity.getAutocheckChunkId();
        this.name = entity.getName();
        this.subtestName = entity.getSubtestName();
        this.uidChanged = entity.isUidChanged();
        this.tags = entity.getTags();
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
        this.important = true;
        this.strongModeAYaml = entity.getStrongModeAYaml();
        this.isMuted = entity.isMuted();
        this.created = entity.getCreated();
        this.statistics = entity.getStatistics();
        this.processedBy = entity.getProcessedBy();
        this.aggregateIdHash = entity.getAggregateIdHash();
        this.chunkId = entity.getChunkId();
        this.isOwner = entity.getIsOwner();
        this.owners = entity.getOwners();
    }

    public String getOldTestId() {
        return oldTestId == null ? "" : oldTestId;
    }

    public String getOldSuiteId() {
        return oldSuiteId == null ? "" : oldSuiteId;
    }

    @Override
    public Id getId() {
        return id;
    }

    public boolean isStrongMode() {
        return isStrongMode != null && isStrongMode;
    }

    public boolean isLaunchable() {
        return isLaunchable != null && isLaunchable;
    }

    public String getStrongModeAYaml() {
        return Objects.requireNonNullElse(strongModeAYaml, "");
    }

    public boolean getImportant() {
        return important != null && important;
    }

    public List<String> getTagsList() {
        return TagUtils.fromTagsString(tags);
    }

    public long getAutocheckChunkId() {
        return Objects.requireNonNullElse(autocheckChunkId, 0L);
    }

    public Id getAutocheckChunkDiffId() {
        return this.id.toBuilder()
                .testId(getAutocheckChunkId())
                .build();
    }

    public boolean isChunk() {
        return this.getAutocheckChunkId() == this.id.testId;
    }

    public boolean isOwner() {
        return isOwner != null && isOwner;
    }

    public ResultOwners getOwners() {
        return owners == null ? ResultOwners.EMPTY : owners;
    }

    @lombok.Builder(toBuilder = true)
    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestDiffEntity> {
        CheckEntity.Id checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        ResultType resultType;

        String toolchain;

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

        public Id(TestDiffImportantEntity.Id id) {
            this.checkId = id.getCheckId();
            this.iterationType = id.getIterationType();
            this.iterationNumber = id.getIterationNumber();
            this.resultType = id.getResultType();
            this.path = id.getPath();
            this.toolchain = id.getToolchain();
            this.suiteId = id.getSuiteId();
            this.testId = id.getTestId();
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

        public Id toSuiteId() {
            return this.toBuilder()
                    .testId(suiteId)
                    .resultType(ResultTypeUtils.toSuiteType(resultType))
                    .build();
        }
    }

    public int getAggregateIdHash() {
        return Objects.requireNonNullElse(aggregateIdHash, 0);
    }
}
