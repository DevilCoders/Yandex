package ru.yandex.ci.storage.core.db.model.test_statistics;

import java.util.function.Function;

import javax.annotation.Nonnull;

import com.google.common.primitives.UnsignedLongs;
import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Builder.Default;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatistics;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@With
@Table(name = "TestStatistics")
public class TestStatisticsEntity implements Entity<TestStatisticsEntity> {

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestStatisticsEntity.class,
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

    @Column(flatten = false)
    @Default
    TestStatistics statistics = TestStatistics.EMPTY;

    @Nonnull
    @Override
    public Id getId() {
        return id;
    }

    @SuppressWarnings({"UnstableApiUsage"})
    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestStatisticsEntity> {
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

        public Id(TestStatusEntity.Id id) {
            this.testId = id.getTestId();
            this.suiteId = id.getSuiteId();
            this.branch = id.getBranch();
            this.toolchain = id.getToolchain();
        }

        public TestEntity.Id getFullTestId() {
            return new TestEntity.Id(suiteId, toolchain, testId);
        }

        public static TestStatisticsEntity.Id idInBranch(String branch, TestEntity.Id testId) {
            return new TestStatisticsEntity.Id(branch, testId);
        }

        @Override
        public String toString() {
            return "[%s/%s/%s/%s]".formatted(
                    UnsignedLongs.toString(testId), UnsignedLongs.toString(suiteId), branch, toolchain
            );
        }
    }

    public static Function<Id, TestStatisticsEntity> defaultProvider() {
        return id -> TestStatisticsEntity.builder().id(id).build();
    }
}
