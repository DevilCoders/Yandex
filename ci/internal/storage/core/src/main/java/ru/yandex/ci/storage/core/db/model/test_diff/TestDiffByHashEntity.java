package ru.yandex.ci.storage.core.db.model.test_diff;

import java.time.Instant;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import com.yandex.ydb.table.settings.PartitioningSettings;
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
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test.TestTag;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
@Table(name = "TestDiffsByHash")
// @With // Not supported, because we calculate field `important` in builder
public class TestDiffByHashEntity implements Entity<TestDiffByHashEntity> {
    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestDiffByHashEntity.class, 1024,
                partitions -> ydbTableHint = HintRegistry.uniformAndSplitMerge(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMinPartitionsCount(Math.min(partitions, 6000))
                                .setMaxPartitionsCount(16384)
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
    String path;
    String processedBy;
    String strongModeAYaml;
    String oldTestId;
    String oldSuiteId;

    boolean uidChanged;

    Set<String> tags;
    String requirements;

    TestDiffType diffType;
    ResultType resultType;

    TestStatus left;
    TestStatus right;

    Integer leftIterationNumber;
    Integer rightIterationNumber;

    @Column(dbType = DbType.UINT8)
    Integer clonedFromIteration;

    boolean isMuted;
    boolean isStrongMode;
    boolean isLast; // last diff over all iterations of this type
    @Nullable
    Boolean isLaunchable;

    @Nullable // old values
    Boolean important; // will be copied to TestDiffImportantEntity

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Nullable
    Boolean isOwner;

    @Nullable
    @Column(flatten = false)
    ResultOwners owners; // old owners for notifications

    TestDiffStatistics statistics;

    @Override
    public Id getId() {
        return id;
    }

    public boolean isUnknown() {
        return this.left == TestStatus.TS_UNKNOWN || this.right == TestStatus.TS_UNKNOWN;
    }

    public boolean isExternal() {
        return TestTag.isExternal(tags);
    }

    public String getStrongModeAYaml() {
        return Objects.requireNonNullElse(strongModeAYaml, "");
    }

    public boolean isImportant() {
        return important != null && important;
    }

    @Value
    public static class Id implements Entity.Id<TestDiffByHashEntity> {
        @Column(dbType = "UINT32")
        int aggregateIdHash;

        ChunkAggregateEntity.Id aggregateId;
        TestEntity.Id testId;

        public static Id of(ChunkAggregateEntity.Id aggregateId, TestEntity.Id testId) {
            return new Id(aggregateId.externalHashCode(), aggregateId, testId);
        }

        public Id toSuiteDiffId() {
            return new Id(aggregateIdHash, aggregateId, testId.toSuiteId());
        }

        public Id toAllToolchainsDiffId() {
            return new Id(aggregateIdHash, aggregateId, testId.toAllToolchainsId());
        }

        @Override
        public String toString() {
            return "%s/%s".formatted(aggregateId, testId);
        }

        public Id toChunkId(long autocheckChunkId) {
            return new Id(aggregateIdHash, aggregateId, testId.toAutocheckChunkId(autocheckChunkId));
        }
    }

    @SuppressWarnings("DuplicatedCode")
    public static class Builder {
        public TestDiffByHashEntity build() {
            if (Objects.isNull(id)) {
                throw new IllegalArgumentException("id is null");
            }

            if (Objects.isNull(resultType)) {
                throw new IllegalArgumentException("resultType is null");
            }

            if (Objects.isNull(name)) {
                name = "";
            }

            if (Objects.isNull(subtestName)) {
                subtestName = "";
            }

            if (Objects.isNull(path)) {
                path = "";
            }

            if (Objects.isNull(tags)) {
                tags = Set.of();
            }

            if (Objects.isNull(left)) {
                left = TestStatus.TS_UNKNOWN;
            }

            if (Objects.isNull(right)) {
                right = TestStatus.TS_UNKNOWN;
            }

            if (Objects.isNull(diffType)) {
                if (left != TestStatus.TS_UNKNOWN && right != TestStatus.TS_UNKNOWN) {
                    diffType = TestDiffType.TDT_PASSED;
                } else if (left != TestStatus.TS_UNKNOWN) {
                    diffType = TestDiffType.TDT_DELETED;
                } else {
                    diffType = TestDiffType.TDT_PASSED_NEW;
                }
            }

            if (Objects.isNull(created)) {
                created = Instant.now();
            }

            if (Objects.isNull(statistics)) {
                statistics = TestDiffStatistics.EMPTY;
            }

            if (leftIterationNumber == null) {
                leftIterationNumber = id.getAggregateId().getIterationId().getNumber();
            }

            if (rightIterationNumber == null) {
                rightIterationNumber = id.getAggregateId().getIterationId().getNumber();
            }

            if (strongModeAYaml == null) {
                strongModeAYaml = "";
            }

            if (processedBy == null) {
                processedBy = "";
            }

            important = (important != null && important) || isDiffImportant(); // important flag must never be reset

            return buildInternal();
        }

        private boolean isDiffImportant() {
            if (!ResultTypeUtils.NOT_CHILD_TYPE.contains(resultType)) {
                return false;
            }

            return statistics.getSelf().isImportant() || statistics.getChildren().isImportant();
        }

        public boolean getIsExternal(boolean isOwner) {
            return !isOwner && TestTag.isExternal(tags);
        }

        public boolean getIsMuted() {
            return isMuted;
        }

        public boolean getIsStrongMode() {
            return isStrongMode;
        }
    }
}
