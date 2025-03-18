package ru.yandex.ci.storage.core.db.model.test_status;

import java.time.Instant;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.primitives.UnsignedLongs;
import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Builder.Default;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, builderClassName = "Builder")
@With
@Table(name = "TestStatuses")
@GlobalIndex(
        name = TestStatusEntity.IDX_BRANCH_PATH_TEST_ID,
        fields = {"id.branch", "path", "id.testId", "id.suiteId", "id.toolchain"}
)
@GlobalIndex(
        name = TestStatusEntity.IDX_BRANCH_SERVICE_PATH_TEST_ID,
        fields = {"id.branch", "service", "path", "id.testId", "id.suiteId", "id.toolchain"}
)
@GlobalIndex(name = TestStatusEntity.IDX_OLD_ID, fields = {"oldTestId"})
public class TestStatusEntity implements Entity<TestStatusEntity> {
    public static final String IDX_BRANCH_PATH_TEST_ID = "IDX_BRANCH_PATH_TEST_ID";
    public static final String IDX_BRANCH_SERVICE_PATH_TEST_ID = "IDX_BRANCH_SERVICE_PATH_TEST_ID";
    public static final String IDX_OLD_ID = "IDX_OLD_ID";

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestStatusEntity.class,
                10,
                partitions -> ydbTableHint = HintRegistry.uniformAndSplitMerge(
                        partitions,
                        YdbTableHint.TablePreset.DEFAULT,
                        new PartitioningSettings().setPartitioningBySize(true)
                )
        );
    }

    @Nonnull
    Id id;

    @Nullable
    @Column(dbType = DbType.UINT64)
    Long autocheckChunkId;

    String service;

    @Default
    String oldSuiteId = "";

    @Default
    String oldTestId = "";

    @Nullable
    String uid;

    @Nullable
    String revision;

    long revisionNumber;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant updated;

    @Default
    String path = "";

    @Default
    String name = "";

    @Default
    String subtestName = "";

    @Default
    Set<String> tags = Set.of();

    @Default
    ResultType type = ResultType.RT_BUILD;

    @Default
    TestStatus status = TestStatus.TS_UNKNOWN;

    @Default
    @Nullable
    @Column(flatten = false)
    ResultOwners owners = ResultOwners.EMPTY;

    boolean isMuted;

    @Nonnull
    @Override
    public Id getId() {
        return id;
    }

    public String getUid() {
        return uid == null ? "" : uid;
    }

    public ResultOwners getOwners() {
        return owners == null ? ResultOwners.EMPTY : owners;
    }

    public long getAutocheckChunkId() {
        return Objects.requireNonNullElse(autocheckChunkId, 0L);
    }

    public boolean isChunk() {
        return this.getAutocheckChunkId() == this.id.testId;
    }

    @SuppressWarnings({"UnstableApiUsage"})
    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestStatusEntity> {
        @Column(dbType = DbType.UINT64)
        long testId;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        String branch;

        String toolchain;

        public Id(String branch, TestEntity.Id testId) {
            this.branch = branch;
            this.testId = testId.getId();
            this.suiteId = testId.getSuiteId();
            this.toolchain = testId.getToolchain();
        }

        public static Id idInTrunk(TestEntity.Id testId) {
            return new Id(Trunk.name(), testId);
        }

        public static Id idInBranch(String branch, TestEntity.Id testId) {
            return new Id(branch, testId);
        }

        public boolean isTrunk() {
            return branch.equals(Trunk.name());
        }

        public TestEntity.Id getFullTestId() {
            return new TestEntity.Id(suiteId, toolchain, testId);
        }

        public TestStatusEntity.Id toAllToolchains() {
            return new Id(testId, suiteId, branch, TestEntity.ALL_TOOLCHAINS);
        }

        @Override
        public String toString() {
            return "[%s/%s/%s/%s]".formatted(
                    branch, UnsignedLongs.toString(suiteId), toolchain, UnsignedLongs.toString(testId)
            );
        }

        public Id toSuiteId() {
            return new Id(suiteId, suiteId, branch, toolchain);
        }

        public Id toChunkId(long chunkId) {
            return new Id(chunkId, suiteId, branch, toolchain);
        }
    }

    public static Function<Id, TestStatusEntity> defaultProvider() {
        return id -> TestStatusEntity.builder().id(id).build();
    }

    public static class Builder {
        public String getService() {
            return service == null ? "" : service;
        }
    }
}
