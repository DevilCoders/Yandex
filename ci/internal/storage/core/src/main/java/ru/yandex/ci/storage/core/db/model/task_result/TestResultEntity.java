package ru.yandex.ci.storage.core.db.model.task_result;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Table(name = "TestResults")
@Builder(toBuilder = true, buildMethodName = "buildInternal")
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class TestResultEntity implements Entity<TestResultEntity> {

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestResultEntity.class,
                10000,
                partitions -> ydbTableHint = HintRegistry.uniformAndSplitMerge(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMinPartitionsCount(Math.min(partitions, 10000))
                                .setMaxPartitionsCount(20000)
                                .setPartitionSize(2000)
                )
        );
    }

    Id id;

    @Nullable
    @Column(dbType = DbType.UINT64)
    Long autocheckChunkId;

    @Nullable // old values
    String oldTestId;

    ChunkEntity.Id chunkId;

    Map<String, List<String>> links;
    Map<String, Double> metrics;
    Map<String, TestOutput> testOutputs;

    String name;

    String branch;
    String path;
    String snippet;
    String subtestName;
    String processedBy;

    Set<String> tags;
    String requirements;

    String uid;

    long revisionNumber;

    boolean isRight;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    TestStatus status;

    Common.ResultType resultType;

    @With
    boolean isStrongMode;

    @Nullable
    @Column(flatten = false)
    ResultOwners owners;

    public TestResultEntity(TestResult result) {
        this.id = result.getId();
        this.autocheckChunkId = result.getAutocheckChunkId();

        this.oldTestId = ResultTypeUtils.isTest(result.getResultType()) ?
                result.getOldTestId() : result.getOldSuiteId();

        this.chunkId = result.getChunkId();

        this.links = result.getLinks();
        this.metrics = result.getMetrics();

        this.name = result.getName();

        this.branch = result.getBranch();
        this.path = result.getPath();
        this.snippet = result.getSnippet();
        this.subtestName = result.getSubtestName();
        this.processedBy = result.getProcessedBy();

        this.tags = result.getTags();
        this.requirements = result.getRequirements();

        this.testOutputs = result.getTestOutputs();
        this.uid = result.getUid();

        this.revisionNumber = result.getRevisionNumber();

        this.isRight = result.isRight();

        this.created = result.getCreated();

        this.status = result.getStatus();

        this.resultType = result.getResultType();

        this.isStrongMode = result.isStrongMode();

        this.owners = result.getOwners();
    }

    @Override
    public Id getId() {
        return id;
    }

    public ResultOwners getOwners() {
        return owners == null ? ResultOwners.EMPTY : owners;
    }

    public Long getAutocheckChunkId() {
        return autocheckChunkId == null ? 0 : autocheckChunkId;
    }

    public TestEntity.Id getTestId() {
        return id.getFullTestId();
    }

    public boolean isLeft() {
        return !isRight;
    }

    public TestDiffEntity.Id getDiffId() {
        return new TestDiffEntity.Id(
                id.getIterationId(), this.resultType, this.path, this.id.getFullTestId()
        );
    }

    public String getOldTestId() {
        return oldTestId == null ? "" : oldTestId;
    }

    public static class Builder {
        public TestResultEntity build() {
            Preconditions.checkNotNull(id, "id is null");

            if (metrics == null) {
                metrics = Map.of();
            }

            if (links == null) {
                links = Map.of();
            }

            if (uid == null) {
                uid = "";
            }

            return buildInternal();
        }

    }

    @Value
    @lombok.Builder
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestResultEntity> {
        CheckEntity.Id checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        @Column(dbType = DbType.UINT64)
        Long suiteId;

        @Column(dbType = DbType.UINT64)
        Long testId;

        String toolchain;

        @Column(dbType = DbType.UINT8)
        Integer iterationNumber;

        String taskId;
        Integer partition;
        Integer retryNumber;

        public Id(
                CheckIterationEntity.Id iterationId, TestEntity.Id testId, String taskId, int partition, int retryNumber
        ) {
            this.checkId = iterationId.getCheckId();
            this.iterationType = iterationId.getIterationTypeNumber();
            this.suiteId = testId.getSuiteId();
            this.testId = testId.getId();
            this.toolchain = testId.getToolchain();
            this.iterationNumber = iterationId.getNumber();
            this.taskId = taskId;
            this.partition = partition;
            this.retryNumber = retryNumber;
        }

        public CheckIterationEntity.Id getIterationId() {
            return new CheckIterationEntity.Id(checkId, iterationType, iterationNumber);
        }

        public TestEntity.Id getFullTestId() {
            return new TestEntity.Id(suiteId, toolchain, testId);
        }

        public CheckTaskEntity.Id getFullTaskId() {
            return new CheckTaskEntity.Id(getIterationId(), taskId);
        }
    }
}
